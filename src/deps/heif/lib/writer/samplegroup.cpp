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

#include "samplegroup.hpp"
#include "samplegroupdescriptionbox.hpp"
#include "sampletogroupbox.hpp"

SampleGroup::SampleGroup(const std::string& groupingType) :
    mGroupingType(groupingType)
{
}
