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

#ifndef ITEMPROPERTYCONTAINER_HPP
#define ITEMPROPERTYCONTAINER_HPP

#include "fullbox.hpp"
#include <memory>
#include <vector>

/**
 * @brief Item Property Container class
 * @details 'ipco' box implementation as specified in the HEIF specification.
 * */
class ItemPropertyContainer : public Box
{
public:
    ItemPropertyContainer();
    virtual ~ItemPropertyContainer() = default;

    /**
     * Get Property or FullProperty (here Box or FullBox)
     * @param [in] index 0-based index of the item.
     * @return Base class pointer to the property. nullptr if index is invalid.
     */
    const Box* getProperty(size_t index) const;

    /**
     * Add a Property or FullProperty
     * @param [in] box Pointer to the Box to add
     * @return 0-based index of the new item.
     */
    unsigned int addProperty(std::shared_ptr<Box> box);

    /** Serialize box data to the BitStream.
     *  @see Box::writeBox() */
    virtual void writeBox(BitStream& bitstream);

    /** Deserialize box data from the BitStream.
     *  @see Box::parseBox() */
    virtual void parseBox(BitStream& bitstream);

private:
    /**
     * Find the index of an equivalent property which is already in this ItemPropertyContainer.
     * @todo Currently this method support just 'ispe'. Extending this to cover all property types might need
     *       a different approach.
     * @param [in] box Pointer to the property box to search equivalent for.
     * @return 0-based index of a property if such exists. -1 if not found.
     */
    int getIndex(std::shared_ptr<Box> box) const;

    /**
     * Boxes derived from ItemProperty or ItemFullProperty
     * @todo Preferably unique_ptr should be used here.
     */
    std::vector<std::shared_ptr<Box>> mProperties;
};

#endif /* ITEMPROPERTYCONTAINER_HPP */
