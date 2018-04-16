/*
 * This file is part of Nokia HEIF library
 *
 * Copyright (c) 2015-2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: heif@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its subsidiaries. All rights are reserved.
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior written consent of Nokia.
 *
 *
 */

package com.nokia.heif;

/**
 * A CodedImageItem which contains AVC encoded data.
 * Works just like a CodedImageItem.
 */
public final class AVCImageItem extends CodedImageItem
{

    /**
     * Creates a new AVCImageItem to the given HEIF instance
     * @param heif The parent HEIF instance for the new object
     * @param size Size of the image
     * @throws Exception Thrown if the parent HEIF instance is invalid
     */
    public AVCImageItem(HEIF heif, Size size)
            throws Exception
    {
        super(heif);
        mNativeHandle = createContextNative(heif);
        setSize(size);
    }

    /**
     * Creates a new AVCImageItem to the given HEIF instance with the given image data
     * Also creates a corresponding AVCDecoderConfig object with the given decoder config
     * @param heif The parent HEIF instance for the new object
     * @param size Size of the image
     * @param decoderConfig Byte array containing the decoder config
     * @param imageData Byte array containing the image data as an encoded AVC frame
     * @throws Exception Thrown if the parent HEIF instance is invalid
     */
    public AVCImageItem(HEIF heif, Size size, byte[] decoderConfig, byte[] imageData)
            throws Exception
    {
        this(heif, size);
        DecoderConfig decConfig = new AVCDecoderConfig(heif);

        decConfig.setConfig(decoderConfig);

        setDecoderConfig(decConfig);
        setItemData(imageData);
    }

    /**
     * Creates a new AVCImageItem to the given HEIF instance with the given image data
     * and AVCDecoderConfig
     * @param heif The parent HEIF instance for the new object
     * @param decoderConfig DecoderConfig for the image
     * @param imageData Byte array containing the image data as an encoded AVC frame
     * @throws Exception Thrown if the parent HEIF instance or the decoderconfig is invalid
     */
    public AVCImageItem(HEIF heif, Size size, AVCDecoderConfig decoderConfig, byte[] imageData)
            throws Exception
    {
        this(heif, size);
        checkParameter(decoderConfig);
        setDecoderConfig(decoderConfig);
        setItemData(imageData);
    }

    /**
     * Protected constructor, used to create an object from the native side
     * @param heif The parent HEIF instance for the new object
     * @param nativeHandle A handle to the corresponding C++ object
     */
    protected AVCImageItem(HEIF heif, long nativeHandle)
    {
        super(heif, nativeHandle);
    }

    private native long createContextNative(HEIF heif);
}
