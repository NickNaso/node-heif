/* Copyright (c) 2015-2017, Nokia Technologies Ltd.
 * All rights reserved.
 *
 * Licensed under the Nokia High-Efficiency Image File Format (HEIF) License (the "License").
 *
 * You may not use the High-Efficiency Image File Format except in compliance with the License.
 * The License accompanies the software and can be found in the file "LICENSE.TXT".
 *
 * You may also obtain the License at:
 * https://nokiatech.github.io/heif/license.txt
 */

#include "trackwriter.hpp"

#include "avcsampleentry.hpp"
#include "compositiontodecodebox.hpp"
#include "datastore.hpp"
#include "decodepts.hpp"
#include "editwriter.hpp"
#include "hevcsampleentry.hpp"
#include "mediatypedefs.hpp"
#include "parserfactory.hpp"
#include "parserinterface.hpp"
#include "refsgroup.hpp"
#include "samplegroupdescriptionbox.hpp"
#include "sampletogroupbox.hpp"
#include "services.hpp"
#include "timeutility.hpp"
#include "timewriter.hpp"
#include "writerconstants.hpp"

TrackWriter::TrackWriter(const std::uint32_t trackId, const IsoMediaFile::EditList& editList,
    const std::string& filename, const std::uint32_t tickRate) :
    mTrackBox(new TrackBox),
    mTrackId(trackId),
    mAlterId(0),
    mDisplayTrack(true),
    mPreviewTrack(false),
    mTrackEnabled(true),
    mTrackIsAlter(false),
    mDisplayTimes(),
    mVpsNals(),
    mSpsNals(),
    mPpsNals(),
    mRefsList(),
    mHasPred(false),
    mInternalStore(),
    mDuration(0),
    mDisplayWidth(0),
    mDisplayHeight(0),
    mDisplayRate(0),
    mClockTicks(tickRate),
    mAccessUnitVector(),
    mDecodeOrder(),
    mDisplayOrder(),
    mIsSync(),
    mFilename(filename),
    mEditlist(editList)
{
}

std::unique_ptr<TrackBox> TrackWriter::finalizeWriting()
{
    std::unique_ptr<TrackBox> trackBox(new TrackBox);
    mTrackBox.swap(trackBox);
    return trackBox;
}


void TrackWriter::writeTrackCommon(bool nonOutput)
{
    stszWrite();    // Fill the SampleSizeBox
    stscWrite();    // Fill the SampleToChunkBox
    stcoWrite();    // Fill the ChunkOffsetBox
    stssWrite();    // Fill the SyncSampleTableBox
    timeWrite(nonOutput);    // Fill the TimeToSampleBox, and if needed the CompositionOffsetBox and the CompositionToDecodeBox
    editWrite();    // Fill the EditBox and the contained EditListBoxes

    decodePts();    // Unravel the presentation time for each sample after edits are applied

    drefWrite();    // Fill the DataReferenceBox
    mdhdWrite();    // Fill the MediaHeaderBox
    tkhdWrite();    // Fill the TrackHeaderBox
}


void TrackWriter::setTrackEnabled(const bool value)
{
    mTrackEnabled = value;
}


void TrackWriter::setDisplayTrack(const bool value)
{
    mDisplayTrack = value;
}


void TrackWriter::setPreviewTrack(const bool value)
{
    mPreviewTrack = value;
}


void TrackWriter::setTrackAsAlter(const std::uint32_t value)
{
    mTrackIsAlter = true;
    mAlterId = value;
}

void TrackWriter::registerDataStore()
{
    mInternalStore = std::make_shared<DataStore>();
    DataServe::regStore(mTrackId, mInternalStore);
}

void TrackWriter::storeValue(const std::string& key, const std::string& value)
{
    mInternalStore->setValue(key, value);
}

void TrackWriter::clearVars()
{
    mDisplayTimes.clear();

    mVpsNals.clear();
    mSpsNals.clear();
    mPpsNals.clear();

    mAccessUnitVector.clear();

    mRefsList.clear();
    mDecodeOrder.clear();
    mDisplayOrder.clear();
    mIsSync.clear();
}

void TrackWriter::drefWrite()
{
    DataInformationBox& dinf = mTrackBox->getMediaBox().getMediaInformationBox().getDataInformationBox();
    auto urlBox = std::make_shared<DataEntryUrlBox>();
    urlBox->setFlags(1); // Flag 0x01 tells the data is in this file. DataEntryUrlBox will write only its header.
    dinf.addDataEntryBox(urlBox);
}

void TrackWriter::hdlrWrite(const std::string& handlerType)
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    HandlerBox& handlerBox = mediaBox.getHandlerBox();
    handlerBox.setHandlerType(handlerType);
}


void TrackWriter::mdhdWrite()
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    MediaHeaderBox& mediaHeaderBox = mediaBox.getMediaHeaderBox();

    mediaHeaderBox.setCreationTime(getSecondsSince1904());
    mediaHeaderBox.setModificationTime(getSecondsSince1904());
    mediaHeaderBox.setTimeScale(mClockTicks);
    mediaHeaderBox.setDuration(mDuration);
}


void TrackWriter::stszWrite()
{
    MediaBox& mdia = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mdia.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    SampleSizeBox& stsz = stbl.getSampleSizeBox();

    stsz.setSampleSize(0);
    stsz.setSampleCount(mAccessUnitVector.size());
    std::vector<std::uint32_t> sampleSizes;
    for (auto sampleInfo = mAccessUnitVector.begin(); sampleInfo != mAccessUnitVector.end(); ++sampleInfo)
    {
        sampleSizes.push_back(sampleInfo->mLength);
    }
    stsz.setEntrySize(sampleSizes);
}

void TrackWriter::stscWrite()
{
    MediaBox& mdia = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mdia.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    SampleToChunkBox& stsc = stbl.getSampleToChunkBox();

    SampleToChunkBox::ChunkEntry chunkEntry;
    chunkEntry.firstChunk = 1;
    chunkEntry.samplesPerChunk = mAccessUnitVector.size();
    chunkEntry.sampleDescriptionIndex = 1; // Currently only one sample description per track is assumed
    stsc.addChunkEntry(chunkEntry);
}

void TrackWriter::stcoWrite()
{
    MediaBox& mdia = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mdia.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    ChunkOffsetBox& stco = stbl.getChunkOffsetBox();

    std::vector<std::uint64_t> chunk_offsets;
    chunk_offsets.push_back(8); // Eight bytes (size of mdat header) is the initial chunk offset
    stco.setChunkOffsets(chunk_offsets);
}

void TrackWriter::stssWrite()
{
    MediaBox& mdia = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mdia.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    SyncSampleBox stss;

    std::uint32_t sampleIndex = 1;
    for (auto flag : mIsSync)
    {
        if (flag == true)
        {
            stss.addSample(sampleIndex);
        }
        sampleIndex += 1;
    }
    stbl.setSyncSampleBox(stss);
}

void TrackWriter::stsdWrite(const std::string& codeType, const IsoMediaFile::CodingConstraints& ccst)
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mediaBox.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    SampleDescriptionBox& stsd = stbl.getSampleDescriptionBox();

    // Generate code type specific SampleEntryBox
    std::unique_ptr<SampleEntryBox> sampleEntryBox = nullptr;

    if (codeType == "hvc1")
    {
        sampleEntryBox = getHevcSampleEntry(ccst);
    }
    else if (codeType == "avc1")
    {
        sampleEntryBox = getAvcSampleEntry(ccst);
    }
    else
    {
        throw std::runtime_error("Cannot generate Track SampleEntryBox (unsupported code_type '" + codeType + "')");
    }

    stsd.addSampleEntry(std::move(sampleEntryBox));
}

void TrackWriter::stsdWrite(const std::string& codeType)
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mediaBox.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();
    SampleDescriptionBox& stsd = stbl.getSampleDescriptionBox();

    // Generate code type specific SampleEntryBox
    std::unique_ptr<SampleEntryBox> sampleEntryBox = nullptr;

    if (codeType == "hvc1")
    {
        sampleEntryBox = getHevcSampleEntry();
    }
    else if (codeType == "avc1")
    {
        sampleEntryBox = getAvcSampleEntry();
    }
    else
    {
        throw std::runtime_error("Cannot generate Track SampleEntryBox (unsupported code_type '" + codeType + "')");
    }

    stsd.addSampleEntry(std::move(sampleEntryBox));
}

std::unique_ptr<HevcSampleEntry> TrackWriter::getHevcSampleEntry()
{
    std::unique_ptr<HevcSampleEntry> hevcSampleEntry(new HevcSampleEntry);
    HevcDecoderConfigurationRecord decConf;
    decConf.makeConfigFromSPS(mSpsNals, mDisplayRate);
    decConf.addNalUnit(mVpsNals, HevcNalUnitType::VPS, 0);
    decConf.addNalUnit(mSpsNals, HevcNalUnitType::SPS, 0);
    decConf.addNalUnit(mPpsNals, HevcNalUnitType::PPS, 0);
    hevcSampleEntry->getHevcConfigurationBox().setConfiguration(decConf);
    const std::uint32_t imageWidth = decConf.getPicWidth();
    const std::uint32_t imageHeight = decConf.getPicHeight();

    // All samples are assumed to be in the same file hence the value 1
    hevcSampleEntry->setDataReferenceIndex(1);
    hevcSampleEntry->setWidth(imageWidth);
    hevcSampleEntry->setHeight(imageHeight);

    return hevcSampleEntry;
}


std::unique_ptr<SampleEntryBox> TrackWriter::getHevcSampleEntry(const IsoMediaFile::CodingConstraints& pCcst)
{
    std::unique_ptr<HevcSampleEntry> hevcSampleEntry = getHevcSampleEntry();

    hevcSampleEntry->createCodingConstraintsBox();
    CodingConstraintsBox* ccst = hevcSampleEntry->getCodingConstraintsBox();
    if (!ccst)
    {
        throw std::runtime_error("Coding constraints not found from '" + hevcSampleEntry->getType().getString() + "' box");
    }

    fillCcst(ccst, pCcst);

    return std::move(hevcSampleEntry);
}

std::unique_ptr<AvcSampleEntry> TrackWriter::getAvcSampleEntry()
{
    std::unique_ptr<AvcSampleEntry> avcSampleEntry(new AvcSampleEntry);
    AvcDecoderConfigurationRecord decConf;
    decConf.makeConfigFromSPS(mSpsNals);
    decConf.addNalUnit(mSpsNals, AvcNalUnitType::SPS, 0);
    decConf.addNalUnit(mPpsNals, AvcNalUnitType::PPS, 0);
    avcSampleEntry->getAvcConfigurationBox().setConfiguration(decConf);
    const std::uint32_t imageWidth = decConf.getPicWidth();
    const std::uint32_t imageHeight = decConf.getPicHeight();

    // All samples are assumed to be in the same file hence the value 1
    avcSampleEntry->setDataReferenceIndex(1);
    avcSampleEntry->setWidth(imageWidth);
    avcSampleEntry->setHeight(imageHeight);

    return avcSampleEntry;
}

void TrackWriter::fillCcst(CodingConstraintsBox* ccst, const IsoMediaFile::CodingConstraints& pCcst)
{
    if (mHasPred)
    {
        std::uint8_t maxRefPicUsed = 0;
        for (const auto& refPics : mRefsList)
        {
            if (refPics.size() > maxRefPicUsed)
            {
                maxRefPicUsed = refPics.size();
            }
        }
        ccst->setMaxRefPicUsed(maxRefPicUsed);
    }

    ccst->setAllRefPicsIntra(pCcst.allRefPicsIntra);
    ccst->setIntraPredUsed(pCcst.intraPredUsed);
}

std::unique_ptr<SampleEntryBox> TrackWriter::getAvcSampleEntry(const IsoMediaFile::CodingConstraints& pCcst)
{
    std::unique_ptr<AvcSampleEntry> avcSampleEntry = getAvcSampleEntry();

    avcSampleEntry->createCodingConstraintsBox();
    CodingConstraintsBox* ccst = avcSampleEntry->getCodingConstraintsBox();
    if (!ccst)
    {
        throw std::runtime_error("Coding constraints not found from '" + avcSampleEntry->getType().getString() + "' box");
    }

    fillCcst(ccst, pCcst);

    return std::move(avcSampleEntry);
}

void TrackWriter::timeWrite(const bool nonOutput)
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    MediaInformationBox& mediaInformationBox = mediaBox.getMediaInformationBox();
    SampleTableBox& sampleTableBox = mediaInformationBox.getSampleTableBox();
    TimeToSampleBox& timeToSampleBox = sampleTableBox.getTimeToSampleBox();

    TimeWriter timeWriter(mClockTicks);
    bool isDisplayRateGiven = (mDisplayRate > 0) ? true : false;
    if (isDisplayRateGiven == true) // Assume constant display rate
    {
        timeWriter.setDisplayRate(mDisplayRate);
    }
    timeWriter.loadOrder(mDecodeOrder, mDisplayOrder, nonOutput);
    timeWriter.fillTimeToSampleBox(timeToSampleBox);

    // If ctts box is to be written
    if (timeWriter.isCompositionOffsetBoxRequired() == true)
    {
        CompositionOffsetBox compositionOffsetBox;
        timeWriter.fillCompositionOffsetBox(compositionOffsetBox);
        sampleTableBox.setCompositionOffsetBox(compositionOffsetBox);
    }

    // If cslg box is to be written
    if (timeWriter.isCompositionToDecodeBoxRequired() == true)
    {
        CompositionToDecodeBox compositionToDecodeBox;
        timeWriter.fillCompositionToDecodeBox(compositionToDecodeBox, nonOutput);
        sampleTableBox.setCompositionToDecodeBox(compositionToDecodeBox);
    }
}


void TrackWriter::editWrite()
{
    if (mEditlist.edit_unit.size() > 0)
    {
        EditWriter editWriter(mClockTicks, mEditlist);
        EditBox editBox;
        editWriter.editWrite(editBox);
        mTrackBox->setEditBox(editBox);
    }
}


void TrackWriter::decodePts()
{
    MediaBox& mediaBox = mTrackBox->getMediaBox();
    MediaInformationBox& mediaInformationBox = mediaBox.getMediaInformationBox();
    SampleTableBox& sampleTableBox = mediaInformationBox.getSampleTableBox();

    const TimeToSampleBox* timeToSampleBox = &(sampleTableBox.getTimeToSampleBox());
    const CompositionOffsetBox* compositionOffsetBox = sampleTableBox.getCompositionOffsetBox().get();
    const CompositionToDecodeBox* compositionToDecodeBox = sampleTableBox.getCompositionToDecodeBox().get();

    DecodePts decodePts;
    decodePts.loadBox(timeToSampleBox);
    if (compositionOffsetBox != nullptr)
    {
        decodePts.loadBox(compositionOffsetBox);
    }
    if (compositionToDecodeBox != nullptr)
    {
        decodePts.loadBox(compositionToDecodeBox);
    }

    std::shared_ptr<const EditBox> editBox = mTrackBox->getEditBox();
    if (editBox)
    {
        const EditListBox* editListBox = editBox->getEditListBox();
        if (editListBox != nullptr)
        {
            decodePts.loadBox(editListBox);
        }
    }
    decodePts.unravel();

    const bool editUnitsPresent = (mEditlist.edit_unit.size() > 0);
    const bool infiniteLooping = (editUnitsPresent && mEditlist.numb_rept == -1);

    if (infiniteLooping)
    {
        mDuration = 0xffffffff;
    }
    else
    {
        mDuration = decodePts.getSpan();

        // Set total duration based on repetitions
        if (editUnitsPresent)
        {
            mDuration = mDuration * (mEditlist.numb_rept + 1);
        }
    }
}


uint8_t TrackWriter::getNalStartCodeSize(const std::vector<uint8_t>& nalU) const
{
    uint8_t size = 0;
    for (auto byte : nalU)
    {
        if (byte == 0)
        {
            ++size;
        }
        if (byte == 1)
        {
            ++size;
            break;
        }
    }
    return size;
}

void TrackWriter::bstrParse(const std::string& codeType)
{
    // Create bitstream parser for this code type
    MediaType mediaType = MediaTypeTool::getMediaTypeByCodeType(codeType, mFilename);
    std::unique_ptr<ParserInterface> mediaParser = ParserFactory::getParser(mediaType);

    if (!mediaParser->openFile(mFilename.c_str()))
    {
        throw std::runtime_error("Not able to open " + MediaTypeTool::getBitStreamTypeName(mediaType) +
                                 " bit stream file '" + mFilename + "'");
    }

    ParserInterface::AccessUnit* accessUnit = new ParserInterface::AccessUnit { };

    bool hasNalUnits = (accessUnit->mNalUnits.size() > 0) ? true : false;
    bool hasMoreImages = (mediaParser->parseNextAU(*accessUnit));
    bool hasSpsNalUnits = (accessUnit->mSpsNalUnits.size() > 0) ? true : false;
    bool hasPpsNalUnits = (accessUnit->mPpsNalUnits.size() > 0) ? true : false;

    bool hasMediaData = (hasMoreImages || hasNalUnits || hasSpsNalUnits || hasPpsNalUnits);

    // Handle AVC & HEVC media data
    if (hasMediaData && ((mediaType == MediaType::AVC) || (mediaType == MediaType::HEVC)))
    {
        if (mediaType == MediaType::HEVC)
        {
            mVpsNals = accessUnit->mVpsNalUnits.front();
        }
        mSpsNals = accessUnit->mSpsNalUnits.front();
        mPpsNals = accessUnit->mPpsNalUnits.front();

        mAccessUnitVector.clear();
        uint32_t sampleOffset = 0;
        while (hasMoreImages)
        {
            if (accessUnit == nullptr)
            {
                accessUnit = new ParserInterface::AccessUnit { };
                hasMoreImages = mediaParser->parseNextAU(*accessUnit);
            }
            if (hasMoreImages)
            {
                AccessUnitInfo sampleInfo;
                uint32_t sampleLength = 0;
                for (auto nalU : accessUnit->mNalUnits)
                {
                    sampleLength += nalU.size() + 4 - getNalStartCodeSize(nalU);
                }

                // Mark as using predicted pictures if there are picture other than intra or idr
                if (not mHasPred)
                {
                    mHasPred = (accessUnit->mIsIntra || accessUnit->mIsIdr) ? false : true;
                }

                // Push references for this picture into the reference list vector
                mRefsList.push_back(accessUnit->mRefPicIndices);
                mDecodeOrder.push_back(accessUnit->mPicIndex);
                mDisplayOrder.push_back(accessUnit->mDisplayOrder);
                mIsSync.push_back((accessUnit->mIsIdr == true) ? true : false);

                sampleInfo.mLength = sampleLength;
                sampleInfo.mOffset = sampleOffset;
                sampleOffset = sampleOffset + sampleLength;
                mAccessUnitVector.push_back(sampleInfo);
            }
            delete accessUnit;
            accessUnit = nullptr;
        }
    }
}

void TrackWriter::setDisplayWidth(const std::uint32_t width)
{
    mDisplayWidth = width;
}

void TrackWriter::setDisplayHeight(const std::uint32_t height)
{
    mDisplayHeight = height;
}

void TrackWriter::setDisplayRate(const std::uint32_t rate)
{
    mDisplayRate = rate;
}

void TrackWriter::setClockTicks(const std::uint32_t clockticks)
{
    mClockTicks = clockticks;
}

uint32_t TrackWriter::getTrackId() const
{
    return mTrackId;
}

TrackBox* TrackWriter::getTrackBox()
{
    return mTrackBox.get();
}

void TrackWriter::tkhdWrite()
{
    TrackHeaderBox& tkhd = mTrackBox->getTrackHeaderBox();

    std::uint32_t flag = 0;
    // If track is enabled
    if (mTrackEnabled == true)
    {
        flag = flag | 0x00000001;
    }
    // If track is used for presentation
    if (mDisplayTrack == true)
    {
        flag = flag | 0x00000002;
    }
    // If track is a preview track
    if (mPreviewTrack == true)
    {
        flag = flag | 0x00000004;
    }
    tkhd.setFlags(flag);

    // If this is an alternative track
    if (mTrackIsAlter == true)
    {
        tkhd.setAlternateGroup(mAlterId);
    }

    tkhd.setCreationTime(getSecondsSince1904());
    tkhd.setModificationTime(getSecondsSince1904());
    tkhd.setTrackID(mTrackId);
    tkhd.setDuration(mDuration);
    tkhd.setWidth(mDisplayWidth << 16);
    tkhd.setHeight(mDisplayHeight << 16);
}


void TrackWriter::sgrpWrite()
{
    MediaBox& mdia = mTrackBox->getMediaBox();
    MediaInformationBox& minf = mdia.getMediaInformationBox();
    SampleTableBox& stbl = minf.getSampleTableBox();

    // If predicted samples are found then the ReferencedPictureSampleGroup is
    // constructed.
    if (mHasPred == true)
    {
        std::unique_ptr<SampleGroupDescriptionBox> sgpd(new SampleGroupDescriptionBox);
        SampleToGroupBox& sbgp = stbl.getSampleToGroupBox();

        RefsGroup refsGroup;
        refsGroup.loadRefs(mRefsList);
        refsGroup.fillSgpd(sgpd.get());
        refsGroup.fillSbgp(sbgp);

        stbl.setSampleGroupDescriptionBox(std::move(sgpd));
    }
}
