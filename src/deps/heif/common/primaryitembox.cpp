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

#include "primaryitembox.hpp"

PrimaryItemBox::PrimaryItemBox() :
    FullBox("pitm", 0, 0),
    mItemId(0)
{
}

void PrimaryItemBox::setItemId(uint32_t itemId)
{
    mItemId = itemId;
}

uint32_t PrimaryItemBox::getItemId() const
{
    return mItemId;
}

void PrimaryItemBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    if (getVersion() == 0)
    {
        bitstr.write16Bits(mItemId);
    }
    else
    {
        bitstr.write32Bits(mItemId);
    }
    updateSize(bitstr);
}

void PrimaryItemBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    if (getVersion() == 0)
    {
        mItemId = bitstr.read16Bits();
    }
    else
    {
        mItemId = bitstr.read32Bits();
    }
}
