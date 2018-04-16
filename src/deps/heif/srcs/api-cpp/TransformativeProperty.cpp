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

#include "TransformativeProperty.h"
#include <heifreader.h>
#include <heifwriter.h>

using namespace HEIFPP;
TransformativeProperty::TransformativeProperty(Heif* aHeif, const HEIF::ItemPropertyType& aType)
    : ItemProperty(aHeif, aType, true){};
HEIF::ErrorCode TransformativeProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    return ItemProperty::load(aReader, aId);
}

CleanApertureProperty::CleanApertureProperty(Heif* aHeif)
    : TransformativeProperty(aHeif, HEIF::ItemPropertyType::CLAP)
    , mClap{}
{
}
HEIF::ErrorCode CleanApertureProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = TransformativeProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mClap);
    }
    return error;
};
HEIF::ErrorCode CleanApertureProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mClap, mId);
}

RotateProperty::RotateProperty(Heif* aHeif)
    : TransformativeProperty(aHeif, HEIF::ItemPropertyType::IROT)
    , mRotate{0}
{
}
HEIF::ErrorCode RotateProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = TransformativeProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mRotate);
    }
    return error;
};
HEIF::ErrorCode RotateProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mRotate, mId);
}

MirrorProperty::MirrorProperty(Heif* aHeif)
    : TransformativeProperty(aHeif, HEIF::ItemPropertyType::IMIR)
    , mMirror{0}
{
}
HEIF::ErrorCode MirrorProperty::load(HEIF::Reader* aReader, const HEIF::PropertyId& aId)
{
    HEIF::ErrorCode error;
    error = TransformativeProperty::load(aReader, aId);
    if (HEIF::ErrorCode::OK == error)
    {
        error = aReader->getProperty(aId, mMirror);
    }
    return error;
};

HEIF::ErrorCode MirrorProperty::save(HEIF::Writer* aWriter)
{
    return aWriter->addProperty(mMirror, mId);
}
