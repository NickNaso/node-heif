/* Copyright (c) 2015, Nokia Technologies Ltd.
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

#include "ftypwriter.hpp"

FtypWriter::FtypWriter(const IsoMediaFile::Brands& brandConf) :
    mBrandConf(brandConf)
{
}

FileTypeBox& FtypWriter::writeBox(BitStream& bitStream)
{
    mFtypBox.setMajorBrand(mBrandConf.major);
    mFtypBox.setMinorVersion(0);

    for (const auto& brand : mBrandConf.other)
    {
        mFtypBox.addCompatibleBrand(brand);
    }

    mFtypBox.writeBox(bitStream);

    return mFtypBox;
}
