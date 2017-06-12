# HEIF
## Overview of the High Efficiency Image File Format

## Abstract

This document presents an overview of the High Efficiency Image File Format (HEIF, ISO/IEC 23008-12). HEIF specifies the storage of individual images as well as image sequences into a container file. HEIF includes the storage specification of HEVC intra images and HEVC image sequences in which inter prediction is applied in a constrained manner. HEIF files are compatible with the ISO Base Media File Format (ISO/IEC 14496-12) and can also include other media streams, such as timed text and audio.
This contribution updates the overview provided in JCTVC-U0039 to be aligned with the decisions made in MPEG meeting #112 (June 2015) and hence with the final technical design of HEIF.

## Introduction
The High Efficiency Image File Format (HEIF, ISO/IEC 23008-12) enables encapsulation of images and image sequences, as well as their associated metadata into a container file. HEIF is compatible with the ISO Base Media File Format (ISOBMFF, ISO/IEC 14496-12). Use cases supported by HEIF include:

 * Storage of burst photos.
 * Support for simultaneous capture of video and still images, i.e. storing still images and timed image sequences into the same file.
 * Efficient representation of animations and cinemagraphs.
 * Storage of focal and exposure stacks into the same container file.
 * Storage of images derived from other images of the file, either as derived images represented by non-destructive image editing operations or as pre-computed derived images.
 * Support for storing other media, such as audio and timed text, into the same container file with timed image sequences and synchronizing their playback.

HEIF specifies a **structural format**, from which codec-specific image formats can be derived. HEIF also includes the specification for encapsulating images and image sequences conforming to the High Efficiency Video Coding (HEVC, ISO/IEC 23008-2 | ITU-T Rec. H.265).

In ISOBMFF, a continuous or timed media or metadata stream forms a track, whereas static media or metadata is stored as items. Consequently, HEIF has the following basic design:

 1. Still images are stored as items. All image items are independently coded and do not depend on any other item in their decoding. Any number of image items can be included in the same file
 2. Image sequences are stored as tracks. An image sequence track can be indicated to be displayed either as a timed sequence or in a non-timed manner, such as a gallery of images. An image sequence track needs to be used instead of image items when there is coding dependency between images.

A file may contain both image items and image sequence tracks along with other media. For example, it is possible to create a file that includes image items or image sequence tracks conforming to HEIF, along with video, audio and timed text tracks conforming to any derivative format of the ISOBMFF.

Files conforming to ISOBMFF consist of a sequence of data structures called boxes, each containing a four-character code (4CC) indicating the type of the box, the size of the box in terms of bytes, and the payload of the box. Boxes may be nested, i.e. a box may contain other boxes. ISOBMFF and HEIF specify constraints on the allowed box order and hierarchy.
