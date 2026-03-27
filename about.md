# GD FastLoad (QOI Cache)

A Geometry Dash modification designed to significantly reduce game loading times by implementing a QOI (Quite OK Image Format) caching system for textures.

## Performance Results

By bypassing standard image decoding and utilizing the QOI format, the resource loading phase is optimized as follows:

| State        | Loading Time |  Improvement |
|:-------------|:------------:|-------------:|
| Vanilla      |    2.69s     |            — |
| **With Mod** |  **1.40s**   |      **~48%** |

## Technical Overview

The mod hooks the CCImage initialization process to optimize asset loading:

1. **Initial Run**: The mod loads standard .png files and automatically generates a cached copy in .qoi format.
2. **Subsequent Runs**: The mod detects the existing cache and decodes the .qoi files directly into memory.
3. **Efficiency**: QOI decoding is significantly faster than PNG, effectively reducing CPU overhead during the loading sequence to near disk-read speeds.

## Implementation Details

* **Framework**: C++ (Geode SDK)
* **Library**: qoixx (QOI C++ implementation)
* **Idea**: [GeometryCache](https://github.com/cgytrus/GeometryCache)
* **Validation**: Uses std::filesystem to compare file modification times, ensuring the cache is automatically updated if the source image changes.

## Installation

Install the .geode package via the Geode Mod Index or build from source using the provided CMake configuration.
