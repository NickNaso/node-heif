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

#pragma once

#include <DerivedImageItem.h>
namespace HEIFPP
{
    class Overlay : public HEIFPP::DerivedImageItem
    {
    public:
        Overlay(Heif* aHeif);
        virtual ~Overlay() = default;

        /** Returns the red component of the background color. The RGB values are in the sRGB color space as defined in
         * IEC 61966-2-1.
         * @return uint16_t: Color value in the range of 0 to 65535 */
        uint16_t r() const;

        /** Returns the green component of the background color. The RGB values are in the sRGB color space as defined
         * in IEC 61966-2-1.
         * @return uint16_t: Color value in the range of 0 to 65535 */
        uint16_t g() const;

        /** Returns the blue component of the background color. The RGB values are in the sRGB color space as defined in
         * IEC 61966-2-1.
         * @return uint16_t: Color value in the range of 0 to 65535 */
        uint16_t b() const;

        /** Returns the alpha component of the background color. The RGB values are in the sRGB color space as defined
         * in IEC 61966-2-1.
         * @return uint16_t: Color value in the range of 0 to 65535 */
        uint16_t a() const;

        /** Sets the red component for the background color
         * @param [in] aR: The value in the range of 0 to 65535 */
        void setR(uint16_t aR);

        /** Sets the green component for the background color
         * @param [in] aR: The value in the range of 0 to 65535 */
        void setG(uint16_t aG);

        /** Sets the blue component for the background color
         * @param [in] aR: The value in the range of 0 to 65535 */
        void setB(uint16_t aB);

        /** Sets the alpha component for the background color
         * @param [in] aR: The value in the range of 0 to 65535 */
        void setA(uint16_t aA);

        /** Returns the count of images in the overlay */
        uint32_t imageCount() const;

        /** Returns the image with the given index
         * @param [in] aIndex: Index of the image
         * @param [out] aOffset: The offset information for the image */
        ImageItem* getImage(uint32_t aIndex, HEIF::Overlay::Offset& aOffset);
        const ImageItem* getImage(uint32_t aIndex, HEIF::Overlay::Offset& aOffset) const;

        /** Adds an image to the overlay
         * @param [in] aItem: Image to be added
         * @param [in] aOffset: The offset information for the image */
        void addImage(ImageItem* aItem, const HEIF::Overlay::Offset& aOffset);

        /** Sets an image to the given index
         * @param [in] aIndex: Index to be set
         * @param [in] aItem: The image to be set
         * @param [in] aOffset: Offset information for the image */
        Result setImage(uint32_t aIndex, ImageItem* aItem, const HEIF::Overlay::Offset& aOffset);

        /** Removes an image with the give index
         * @param [in] aIndex: The index to be removed */
        Result removeImage(uint32_t aIndex);

        /** Removes an image from the overlay
         * @param [in] aImage: Image to be removed */
        Result removeImage(ImageItem* aImage);

    protected:
        virtual HEIF::ErrorCode load(HEIF::Reader* aReader, const HEIF::ImageId& aId);
        virtual HEIF::ErrorCode save(HEIF::Writer* aWriter);

        uint16_t mR, mG, mB, mA;
        std::vector<HEIF::Overlay::Offset> mOffsets;

    private:
        Overlay& operator=(const Overlay&) = delete;
        Overlay(const Overlay&)            = delete;
        Overlay(Overlay&&)                 = delete;
        Overlay()                          = delete;
    };

}  // namespace HEIFPP
