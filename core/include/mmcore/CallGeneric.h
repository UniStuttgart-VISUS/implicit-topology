/*
 * CallGeneric.h
 *
 * Copyright (C) 2019 by Universitaet Stuttgart (VISUS).
 * All rights reserved.
 */

#ifndef CALL_GENERIC_H_INCLUDED
#define CALL_GENERIC_H_INCLUDED

#include "mmcore/AbstractGetDataCall.h"
#include "mmcore/BoundingBoxes_2.h"

namespace megamol {
namespace core {

/**
 * The most basic meta data only features a hash value to indicate when "something has changed".
 */
struct BasicMetaData {
    size_t m_data_hash = 0;
};

/**
 * Meta data for spatial 3D data communicates the data bounding box as well as frame count
 * and current frame ID for time dependent data.
 * A hash value provides the possibility to communicate when "something has changed".
 */
struct Spatial3DMetaData {
    size_t m_data_hash = 0;
    unsigned int m_frame_cnt = 0;
    unsigned int m_frame_ID = 0;
    megamol::core::BoundingBoxes_2 m_bboxs;
};

template <typename DataType, typename MetaDataType> class CallGeneric : public Call 
{
public:

    using data_type = DataType;
    using meta_data_type = MetaDataType;

    CallGeneric() = default;
    ~CallGeneric() = default;

    static unsigned int FunctionCount() { return 2; }

    static const char* FunctionName(unsigned int idx) {
        switch (idx) {
        case 0:
            return "GetData";
        case 1:
            return "GetMetaData";
        }
        return NULL;
    }

    void setData(DataType const& data) { m_data = data; }

    void setMetaData(MetaDataType const& meta_data) { m_meta_data = meta_data; }

    DataType const& getData() { return m_data; }

    MetaDataType const& getMetaData() { return m_meta_data; }

private:
    DataType     m_data;
    MetaDataType m_meta_data;
};

} // namespace mesh
} // namespace megamol

#endif // !CALL_GENERIC_H_INCLUDED
