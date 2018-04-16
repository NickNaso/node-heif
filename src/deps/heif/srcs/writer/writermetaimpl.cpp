/* This file is part of Nokia HEIF library
 *
 * Copyright (c) 2015-2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: heif@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include "auxiliarytypeproperty.hpp"
#include "avcconfigurationbox.hpp"
#include "cleanaperturebox.hpp"
#include "colourinformationbox.hpp"
#include "customallocator.hpp"
#include "heifallocator.h"
#include "hevcconfigurationbox.hpp"
#include "imagegrid.hpp"
#include "imagemirror.hpp"
#include "imageoverlay.hpp"
#include "imagerelativelocationproperty.hpp"
#include "imagerotation.hpp"
#include "imagespatialextentsproperty.hpp"
#include "jpegparser.hpp"
#include "pixelaspectratiobox.hpp"
#include "pixelinformationproperty.hpp"
#include "rawpropertybox.hpp"
#include "writerimpl.hpp"

using namespace std;

namespace HEIF
{
    namespace
    {
        ErrorCode createAvcDecoderConfigurationRecord(const Array<DecoderSpecificInfo>& nalUnits,
                                                      AvcDecoderConfigurationRecord& configRecord)
        {
            bool spsFound = false;
            bool ppsFound = false;

            for (auto& nalUnit : nalUnits)
            {
                const auto nalVector = vectorize(nalUnit.decSpecInfoData);

                if (nalUnit.decSpecInfoType == DecoderSpecInfoType::AVC_PPS && !ppsFound)
                {
                    ppsFound = true;
                    configRecord.addNalUnit(nalVector, AvcNalUnitType::PPS);
                }
                else if (nalUnit.decSpecInfoType == DecoderSpecInfoType::AVC_SPS && !spsFound)
                {
                    spsFound = true;
                    configRecord.addNalUnit(nalVector, AvcNalUnitType::SPS);
                    configRecord.makeConfigFromSPS(nalVector);
                }
                else
                {
                    return ErrorCode::DECODER_CONFIGURATION_ERROR;
                }
            }

            if (!spsFound || !ppsFound)
            {
                return ErrorCode::DECODER_CONFIGURATION_ERROR;
            }

            return ErrorCode::OK;
        }

        ErrorCode createHevcDecoderConfigurationRecord(const Array<DecoderSpecificInfo>& nalUnits,
                                                       HevcDecoderConfigurationRecord& configRecord)
        {
            bool spsFound = false;
            bool ppsFound = false;
            bool vpsFound = false;

            for (auto& nalUnit : nalUnits)
            {
                const auto nalVector = vectorize(nalUnit.decSpecInfoData);

                if (nalUnit.decSpecInfoType == DecoderSpecInfoType::HEVC_PPS && !ppsFound)
                {
                    ppsFound = true;
                    configRecord.addNalUnit(nalVector, HevcNalUnitType::PPS, true);
                }
                else if (nalUnit.decSpecInfoType == DecoderSpecInfoType::HEVC_VPS && !vpsFound)
                {
                    vpsFound = true;
                    configRecord.addNalUnit(nalVector, HevcNalUnitType::VPS, true);
                }
                else if (nalUnit.decSpecInfoType == DecoderSpecInfoType::HEVC_SPS && !spsFound)
                {
                    spsFound = true;
                    configRecord.addNalUnit(nalVector, HevcNalUnitType::SPS, true);
                    configRecord.makeConfigFromSPS(nalVector, 0.0);
                }
                else
                {
                    return ErrorCode::DECODER_CONFIGURATION_ERROR;
                }
            }

            if (!spsFound || !ppsFound || !vpsFound)
            {
                return ErrorCode::DECODER_CONFIGURATION_ERROR;
            }

            return ErrorCode::OK;
        }
    }  // anonymous namespace

    /* *************************************************************** */
    /* ********************* Writer API  methods ********************* */
    /* *************************************************************** */

    ErrorCode WriterImpl::setPrimaryItem(const ImageId& imageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({imageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        if (mImageCollection.images[imageId].isHidden)
        {
            return ErrorCode::HIDDEN_PRIMARY_ITEM;
        }

        mMetaBox.setPrimaryItem(imageId.get());
        mPrimaryItemSet = true;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addImage(const MediaDataId& aMediaDataId, ImageId& aImageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (mMediaData.count(aMediaDataId) == 0)
        {
            return ErrorCode::INVALID_MEDIADATA_ID;
        }

        aImageId = Context::getValue();

        ImageCollection::Image newImage;
        newImage.imageId                  = aImageId;
        mImageCollection.images[aImageId] = newImage;

        const MediaData& mediaData = mMediaData.at(aMediaDataId);

        struct FormatNames
        {
            FourCCInt type;
            String name;
        };
        const map<MediaFormat, FormatNames> formatMapping = {{MediaFormat::AVC, {FourCCInt("avc1"), "AVC Image"}},
                                                             {MediaFormat::HEVC, {FourCCInt("hvc1"), "HEVC Image"}},
                                                             {MediaFormat::JPEG, {FourCCInt("jpeg"), "JPEG Image"}}};

        const FormatNames& format = formatMapping.at(mediaData.mediaFormat);

        mMetaBox.addItem(newImage.imageId.get(), format.type, format.name);
        mMetaBox.addIloc(newImage.imageId.get(), mediaData.offset, mediaData.size, 0);

        if ((mediaData.mediaFormat == MediaFormat::AVC) || (mediaData.mediaFormat == MediaFormat::HEVC))
        {
            uint16_t configPropertyIndex;
            ErrorCode error = getConfigIndex(mediaData.decoderConfigId, configPropertyIndex);
            if (error != ErrorCode::OK)
            {
                return error;
            }
            const auto imageSize         = mDecoderConfigIndexToSize[configPropertyIndex];
            const auto width             = imageSize.width;
            const auto height            = imageSize.height;
            const auto ispePropertyIndex = getIspeIndex(width, height);
            mMetaBox.addProperty(configPropertyIndex, {aImageId.get()}, true);
            mMetaBox.addProperty(ispePropertyIndex, {aImageId.get()}, false);
        }
        else if (mediaData.mediaFormat == MediaFormat::JPEG)
        {
            const ImageSize& size        = mJpegDimensions.at(aMediaDataId);
            const auto ispePropertyIndex = getIspeIndex(size.width, size.height);
            mMetaBox.addProperty(ispePropertyIndex, {aImageId.get()}, false);
        }

        if (!mInitialMdat)
        {
            mFileTypeBox.addCompatibleBrand("mif1");
        }
        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::setImageHidden(const ImageId& imageId, const bool hidden)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({imageId.get()}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        mMetaBox.setImageHidden(imageId.get(), hidden);
        mImageCollection.images[imageId].isHidden = hidden;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addAuxiliaryReference(const ImageId& fromImageId, const ImageId& toImageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({fromImageId, toImageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        mMetaBox.addItemReference("auxl", fromImageId.get(), toImageId.get());
        return ErrorCode::OK;
    }


    ErrorCode WriterImpl::addThumbnail(const ImageId& thumbImageId, const ImageId& masterImageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({thumbImageId, masterImageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        mMetaBox.addItemReference("thmb", thumbImageId.get(), masterImageId.get());

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::finalizeMetaBox()
    {
        mMetaBox.setHandlerType("pict");

        // If primary item was not set, default to the first non-hidden image.
        if (!mPrimaryItemSet)
        {
            for (const auto& image : mImageCollection.images)
            {
                if (!image.second.isHidden)
                {
                    setPrimaryItem(image.second.imageId);
                    mPrimaryItemSet = true;
                    break;
                }
            }
        }

        for (const auto& entityGroup : mEntityGroups)
        {
            const EntityGroup& group = entityGroup.second;

            Vector<std::uint32_t> itemIds;
            for (auto entity : group.entities)
            {
                itemIds.push_back(entity.id);
            }

            mMetaBox.addEntityGrouping(FourCCInt(group.type.value), entityGroup.first.get(), itemIds);
        }

        for (const auto& entry : mImageCollection.images)
        {
            const ImageCollection::Image& image = entry.second;
            for (const auto& property : image.descriptiveProperties)
            {
                mMetaBox.addProperty(static_cast<uint16_t>(property.propertyId.get()), {image.imageId.get()},
                                     property.essential);
            }
            for (const auto& property : image.transformativeProperties)
            {
                mMetaBox.addProperty(static_cast<uint16_t>(property.propertyId.get()), {image.imageId.get()},
                                     property.essential);
            }
        }

        /** @todo Validity checks?
         * if (!mPrimaryItemSet) still, in case all items were hidden.
         * if (mImageCollection.images.size() == 0) */

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const CleanAperture& clap, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto clapBox = std::make_shared<::CleanApertureBox>();
        ::CleanApertureBox::Fraction value;
        value.numerator   = clap.heightN;
        value.denominator = clap.heightD;
        clapBox->setHeight(value);
        value.numerator   = clap.widthN;
        value.denominator = clap.widthD;
        clapBox->setWidth(value);
        value.numerator   = clap.horizontalOffsetN;
        value.denominator = clap.horizontalOffsetD;
        clapBox->setHorizOffset(value);
        value.numerator   = clap.verticalOffsetN;
        value.denominator = clap.verticalOffsetD;
        clapBox->setVertOffset(value);

        PropertyInformation info;
        info.isTransformative   = true;
        propertyId              = PropertyId(mMetaBox.addProperty(clapBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const Mirror& imir, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto imirBox = std::make_shared<ImageMirror>();
        imirBox->setHorizontalAxis(imir.horizontalAxis);

        PropertyInformation info;
        info.isTransformative   = true;
        propertyId              = PropertyId(mMetaBox.addProperty(imirBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const Rotate& irot, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto irotBox = std::make_shared<ImageRotation>();
        irotBox->setAngle(irot.angle);

        PropertyInformation info;
        info.isTransformative   = true;
        propertyId              = PropertyId(mMetaBox.addProperty(irotBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const RelativeLocation& rloc, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto rlocBox = std::make_shared<ImageRelativeLocationProperty>();
        rlocBox->setHorizontalOffset(rloc.horizontalOffset);
        rlocBox->setVerticalOffset(rloc.verticalOffset);

        PropertyInformation info;
        info.isTransformative   = false;
        propertyId              = PropertyId(mMetaBox.addProperty(rlocBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const PixelAspectRatio& pasp, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto paspBox = std::make_shared<PixelAspectRatioBox>();

        paspBox->setRelativeWidth(pasp.relativeWidth);
        paspBox->setRelativeHeight(pasp.relativeHeight);

        PropertyInformation info;
        info.isTransformative   = false;
        propertyId              = PropertyId(mMetaBox.addProperty(paspBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const PixelInformation& pixi, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto pixiBox = std::make_shared<PixelInformationProperty>();
        Vector<std::uint8_t> bitsPerChannelVector;
        for (const auto bitsPerChannel : pixi.bitsPerChannel)
        {
            bitsPerChannelVector.push_back(bitsPerChannel);
        }
        pixiBox->setBitsPerChannels(bitsPerChannelVector);

        PropertyInformation info;
        info.isTransformative   = false;
        propertyId              = PropertyId(mMetaBox.addProperty(pixiBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const ColourInformation& colr, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto colrBox = std::make_shared<ColourInformationBox>();

        if ((colr.colourType == "nclx") || (colr.colourType == "rICC") || (colr.colourType == "prof"))
        {
            colrBox->setColourType(colr.colourType.value);
        }
        else
        {
            return ErrorCode::INVALID_FUNCTION_PARAMETER;
        }

        colrBox->setColourPrimaries(colr.colourPrimaries);
        colrBox->setTransferCharacteristics(colr.transferCharacteristics);
        colrBox->setMatrixCoefficients(colr.matrixCoefficients);
        colrBox->setFullRangeFlag(colr.fullRangeFlag);
        Vector<std::uint8_t> iccProfileVector;
        for (const auto byte : colr.iccProfile)
        {
            iccProfileVector.push_back(byte);
        }
        colrBox->setIccProfile(iccProfileVector);

        PropertyInformation info;
        info.isTransformative   = false;
        propertyId              = PropertyId(mMetaBox.addProperty(colrBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const AuxiliaryType& auxC, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto auxCBox = std::make_shared<AuxiliaryTypeProperty>();

        String typeUrn(auxC.auxType.begin(), auxC.auxType.end());
        auxCBox->setAuxType(typeUrn);
        if (auxC.subType.size)
        {
            auxCBox->setAuxSubType(vectorize(auxC.subType));
        }

        PropertyInformation info;
        info.isTransformative   = false;
        propertyId              = PropertyId(mMetaBox.addProperty(auxCBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addProperty(const RawProperty& property, const bool isTransformative, PropertyId& propertyId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        auto customBox = std::make_shared<RawPropertyBox>();

        Vector<uint8_t> data(property.data.begin(), property.data.end());
        customBox->setData(data);

        PropertyInformation info;
        info.isTransformative   = isTransformative;
        propertyId              = PropertyId(mMetaBox.addProperty(customBox, {}, false));
        mProperties[propertyId] = info;

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addDerivedImage(const ImageId& imageId, ImageId& derivedImageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({imageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }
        derivedImageId = Context::getValue();
        ImageCollection::Image newImage;
        newImage.isHidden                       = false;
        newImage.imageId                        = derivedImageId;
        mImageCollection.images[derivedImageId] = newImage;


        mMetaBox.addItem(derivedImageId.get(), "iden", "Derived image");
        mMetaBox.addItemReference("dimg", derivedImageId.get(), imageId.get());

        const auto ispeIndex =
            mMetaBox.getItemPropertiesBox().findPropertyIndex(ItemPropertiesBox::PropertyType::ISPE, imageId.get());
        assert(ispeIndex != 0);
        mMetaBox.addProperty(static_cast<uint16_t>(ispeIndex), {derivedImageId.get()}, false);

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::associateProperty(const ImageId& imageId,
                                            const PropertyId& propertyId,
                                            const bool isEssential)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({imageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        if (mProperties.count(propertyId) == 0)
        {
            return ErrorCode::INVALID_PROPERTY_INDEX;
        }

        ImageCollection::PropertyAssociation association;
        association.propertyId = propertyId;
        association.essential  = isEssential;

        if (mProperties.at(propertyId).isTransformative)
        {
            mImageCollection.images.at(imageId).transformativeProperties.push_back(association);
        }
        else
        {
            mImageCollection.images.at(imageId).descriptiveProperties.push_back(association);
        }

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addDerivedImageItem(const Grid& grid, ImageId& gridId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds(grid.imageIds))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        if ((grid.rows * grid.columns) != grid.imageIds.size)
        {
            return ErrorCode::INVALID_REFERENCE_COUNT;
        }

        if (grid.outputHeight == 0 || grid.outputWidth == 0)
        {
            return ErrorCode::INVALID_FUNCTION_PARAMETER;
        }

        gridId = Context::getValue();
        ImageCollection::Image newImage;
        newImage.imageId                = gridId;
        mImageCollection.images[gridId] = newImage;

        ImageGrid gridItem;
        gridItem.columnsMinusOne = static_cast<uint8_t>(grid.columns - 1);
        gridItem.rowsMinusOne    = static_cast<uint8_t>(grid.rows - 1);
        gridItem.outputHeight    = grid.outputHeight;
        gridItem.outputWidth     = grid.outputWidth;

        BitStream bitstream;
        writeImageGrid(gridItem, bitstream);
        mMetaBox.addIdatItem(gridId.get(), "grid", "Derived image", bitstream.getStorage());
        for (const auto toImageId : grid.imageIds)
        {
            mMetaBox.addItemReference("dimg", gridId.get(), toImageId.get());
        }
        const auto ispeIndex = getIspeIndex(gridItem.outputWidth, gridItem.outputHeight);
        mMetaBox.addProperty(ispeIndex, {gridId.get()}, false);

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addDerivedImageItem(const Overlay& iovl, ImageId& overlayId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds(iovl.imageIds))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        if (iovl.offsets.size != iovl.imageIds.size)
        {
            return ErrorCode::INVALID_REFERENCE_COUNT;
        }

        if (iovl.outputHeight == 0 || iovl.outputWidth == 0)
        {
            return ErrorCode::INVALID_FUNCTION_PARAMETER;
        }

        overlayId = Context::getValue();
        ImageCollection::Image newImage;
        newImage.imageId                   = overlayId;
        mImageCollection.images[overlayId] = newImage;

        ImageOverlay iovlItem;
        iovlItem.outputHeight = iovl.outputHeight;
        iovlItem.outputWidth  = iovl.outputWidth;

        Vector<ImageOverlay::Offset> offsets;
        for (const auto& offsetConfig : iovl.offsets)
        {
            ImageOverlay::Offset offset;
            offset.horizontalOffset = offsetConfig.horizontal;
            offset.verticalOffset   = offsetConfig.vertical;
            offsets.push_back(offset);
        }
        iovlItem.offsets          = offsets;
        iovlItem.canvasFillValueR = iovl.r;
        iovlItem.canvasFillValueG = iovl.g;
        iovlItem.canvasFillValueB = iovl.b;
        iovlItem.canvasFillValueA = iovl.a;

        BitStream bitstream;
        writeImageOverlay(iovlItem, bitstream);
        mMetaBox.addIdatItem(overlayId.get(), "iovl", "Derived image", bitstream.getStorage());
        for (const auto toImageId : iovl.imageIds)
        {
            mMetaBox.addItemReference("dimg", overlayId.get(), toImageId.get());
        }
        const auto ispeIndex = getIspeIndex(iovlItem.outputWidth, iovlItem.outputHeight);
        mMetaBox.addProperty(ispeIndex, {overlayId.get()}, false);

        return ErrorCode::OK;
    }


    ErrorCode WriterImpl::addTbasItemReference(const ImageId& fromImageId, const ImageId& toImageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({fromImageId}) || !checkImageIds({toImageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        mMetaBox.addItemReference("tbas", fromImageId.get(), toImageId.get());

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addBaseItemReference(const ImageId& fromImageId, const Array<ImageId>& toImageIds)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({fromImageId}) || !checkImageIds(toImageIds))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        for (const auto toImageId : toImageIds)
        {
            mMetaBox.addItemReference("base", fromImageId.get(), toImageId.get());
        }

        return ErrorCode::OK;
    }

    ErrorCode WriterImpl::addMetadata(const MediaDataId& mediaDataId, const ImageId& imageId)
    {
        if (mState != State::WRITING)
        {
            return ErrorCode::UNINITIALIZED;
        }

        if (!checkImageIds({imageId}))
        {
            return ErrorCode::INVALID_ITEM_ID;
        }

        if (mMediaData.count(mediaDataId) == 0)
        {
            return ErrorCode::INVALID_MEDIADATA_ID;
        }

        MetadataItemId metadataItemId;
        if (createMetadataItem(mediaDataId, metadataItemId) != ErrorCode::OK)
        {
            return ErrorCode::INVALID_MEDIADATA_ID;
        }
        mMetaBox.addItemReference("cdsc", metadataItemId.get(), imageId.get());
        return ErrorCode::OK;
    }

    /* *************************************************************** */
    /* ******************* Internal helper methods ******************* */
    /* *************************************************************** */

    ErrorCode WriterImpl::createMetadataItem(const MediaDataId& mediaDataId, MetadataItemId& metadataItemId)
    {
        if (mMediaData.count(mediaDataId) == 0)
        {
            return ErrorCode::INVALID_MEDIADATA_ID;
        }

        if (!mMetadataItems.count(mediaDataId))
        {
            const MediaData& mediaData = mMediaData.at(mediaDataId);

            struct FormatNames
            {
                FourCCInt type;
                String name;
                String contentType;
            };
            const map<MediaFormat, FormatNames> formatMapping = {
                {MediaFormat::EXIF, {FourCCInt("Exif"), "Exif data", ""}},
                {MediaFormat::MPEG7, {FourCCInt("mime"), "MPEG-7 data", "text/xml"}},
                {MediaFormat::XMP, {FourCCInt("mime"), "XMP data", "application/rdf+xml"}}};
            const FormatNames& format = formatMapping.at(mediaData.mediaFormat);

            mMetadataItems[mediaDataId] = Context::getValue();

            ItemInfoEntry infe;
            infe.setVersion(2);
            infe.setItemType(format.type);
            infe.setItemID(mMetadataItems.at(mediaDataId).get());
            infe.setItemName(format.name);
            infe.setContentType(format.contentType);

            mMetaBox.addItem(infe);
            mMetaBox.addIloc(mMetadataItems.at(mediaDataId).get(), mediaData.offset, mediaData.size, 0);
        }
        metadataItemId = mMetadataItems.at(mediaDataId);
        return ErrorCode::OK;
    }

    bool WriterImpl::checkImageIds(const Array<ImageId>& imageIds) const
    {
        if (imageIds.size == 0)
        {
            return false;
        }

        for (auto imageId : imageIds)
        {
            if (mImageCollection.images.count(imageId) == 0)
            {
                return false;
            }
        }

        return true;
    }

    uint16_t WriterImpl::getIspeIndex(const std::uint32_t width, const std::uint32_t height)
    {
        ImageSize size = {width, height};

        if (mIspeIndexes.count(size) == 0)
        {
            mIspeIndexes[size] =
                mMetaBox.addProperty(std::make_shared<ImageSpatialExtentsProperty>(width, height), {}, false);
        }

        return mIspeIndexes[size];
    }

    ErrorCode WriterImpl::getConfigIndex(const DecoderConfigId decoderConfigId, uint16_t& propertyIndex)
    {
        // Create a new decoder configuration property, if a matching one was not present already.
        if (mDecoderConfigs.count(decoderConfigId) == 0)
        {
            const Array<DecoderSpecificInfo>& configNalUnits = mAllDecoderConfigs.at(decoderConfigId);

            if (configNalUnits.size)
            {
                if (configNalUnits.begin()->decSpecInfoType == AVC_SPS ||
                    configNalUnits.begin()->decSpecInfoType == AVC_PPS)
                {
                    auto configBox = std::make_shared<AvcConfigurationBox>();
                    AvcDecoderConfigurationRecord configRecord;
                    ErrorCode error = createAvcDecoderConfigurationRecord(configNalUnits, configRecord);
                    if (error != ErrorCode::OK)
                    {
                        return error;
                    }

                    configBox->setConfiguration(configRecord);
                    const auto index                 = mMetaBox.addProperty(configBox, {}, true);
                    mDecoderConfigs[decoderConfigId] = index;
                    mDecoderConfigIndexToSize[index] = {configRecord.getPicWidth(), configRecord.getPicHeight()};
                }
                else if (configNalUnits.begin()->decSpecInfoType == HEVC_SPS ||
                         configNalUnits.begin()->decSpecInfoType == HEVC_PPS ||
                         configNalUnits.begin()->decSpecInfoType == HEVC_VPS)
                {
                    auto configBox = std::make_shared<HevcConfigurationBox>();
                    HevcDecoderConfigurationRecord configRecord;
                    ErrorCode error = createHevcDecoderConfigurationRecord(configNalUnits, configRecord);
                    if (error != ErrorCode::OK)
                    {
                        return error;
                    }
                    configBox->setConfiguration(configRecord);
                    const auto index                 = mMetaBox.addProperty(configBox, {}, true);
                    mDecoderConfigs[decoderConfigId] = index;
                    mDecoderConfigIndexToSize[index] = {configRecord.getPicWidth(), configRecord.getPicHeight()};
                }
                else
                {
                    return ErrorCode::UNSUPPORTED_CODE_TYPE;
                }
            }
        }

        propertyIndex = mDecoderConfigs.at(decoderConfigId);
        return ErrorCode::OK;
    }
}  // namespace HEIF
