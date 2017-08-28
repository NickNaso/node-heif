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

#include "protectionschemeinfobox.hpp"
#include "bitstream.hpp"

std::vector<std::uint8_t> ProtectionSchemeInfoBox::getData() const
{
    return mData;
}

void ProtectionSchemeInfoBox::setData(const std::vector<std::uint8_t>& data)
{
    mData = data;
}

void ProtectionSchemeInfoBox::writeBox(BitStream& bitstream)
{
    bitstream.write8BitsArray(mData, mData.size());
}

void ProtectionSchemeInfoBox::parseBox(BitStream& bitstream)
{
    bitstream.read8BitsArray(mData, bitstream.getSize());
}
