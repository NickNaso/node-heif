/*
 * This file is part of Nokia HEIF library
 *
 * Copyright (c) 2015-2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: heif@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved. Copying, including reproducing, storing, adapting or translating, any or all
 * of this material requires the prior written consent of Nokia.
 */

#include "DescriptiveProperty.h"
#include <heifreader.h>
#include <heifwriter.h>
#include <cstring>

using namespace HEIFPP;
DescriptiveProperty::DescriptiveProperty(Heif* aHeif, const HEIF::ItemPropertyType& aProp)
    : ItemProperty(aHeif, aProp, false){};
HEIF::ErrorCode DescriptiveProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    return ItemProperty::load(aReader, aId);
}

PixelAspectRatioProperty::PixelAspectRatioProperty(Heif* aHeif)
    : DescriptiveProperty(aHeif, HEIF::ItemPropertyType::PASP)
    , mPixelAspectRatio{}
{
}
HEIF::ErrorCode PixelAspectRatioProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = DescriptiveProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mPixelAspectRatio);
    }
    return error;
};
HEIF::ErrorCode PixelAspectRatioProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mPixelAspectRatio, mId);
}

ColourInformationProperty::ColourInformationProperty(Heif* aHeif)
    : DescriptiveProperty(aHeif, HEIF::ItemPropertyType::COLR)
    , mColourInformation{}
{
}
HEIF::ErrorCode ColourInformationProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = DescriptiveProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mColourInformation);
    }
    return error;
};
HEIF::ErrorCode ColourInformationProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mColourInformation, mId);
}

PixelInformationProperty::PixelInformationProperty(Heif* aHeif)
    : DescriptiveProperty(aHeif, HEIF::ItemPropertyType::PIXI)
    , mPixelInformation{0}
{
}
HEIF::ErrorCode PixelInformationProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = DescriptiveProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mPixelInformation);
    }
    return error;
};
HEIF::ErrorCode PixelInformationProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mPixelInformation, mId);
}

RelativeLocationProperty::RelativeLocationProperty(Heif* aHeif)
    : DescriptiveProperty(aHeif, HEIF::ItemPropertyType::RLOC)
    , mRelativeLocation{}
{
}
HEIF::ErrorCode RelativeLocationProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = DescriptiveProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mRelativeLocation);
    }
    return error;
}
HEIF::ErrorCode RelativeLocationProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mRelativeLocation, mId);
}

AuxProperty::AuxProperty(Heif* aHeif)
    : DescriptiveProperty(aHeif, HEIF::ItemPropertyType::AUXC)
{
}
HEIF::ErrorCode AuxProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = DescriptiveProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        HEIF::AuxiliaryType p;
        error = aReader->getProperty(aId, p);
        if (HEIF::ErrorCode::OK == error)
        {
            mAuxType = std::string(p.auxType.begin(), p.auxType.end());
            mSubType.assign(p.subType.begin(), p.subType.end());
        }
    }
    return error;
}

HEIF::ErrorCode AuxProperty::save(HEIF::Writer* aWriter)
{
    HEIF::ErrorCode error;
    HEIF::AuxiliaryType p;

    if (mAuxType.length() > 0)
    {
        p.auxType = HEIF::Array<char>(mAuxType.length());
        std::memcpy(p.auxType.elements, mAuxType.data(), p.auxType.size);
    }
    if (mSubType.size() > 0)
    {
        p.subType = HEIF::Array<uint8_t>(mSubType.size());
        std::memcpy(p.subType.elements, mSubType.data(), p.subType.size);
    }
    error = aWriter->addProperty(p, mId);
    return error;
};

const std::string& AuxProperty::auxType()
{
    return mAuxType;
}
void AuxProperty::auxType(const std::string& aType)
{
    mAuxType = aType;
}
const std::vector<uint8_t>& AuxProperty::subType()
{
    return mSubType;
}
void AuxProperty::subType(const std::vector<uint8_t>& aType)
{
    mSubType = aType;
}
