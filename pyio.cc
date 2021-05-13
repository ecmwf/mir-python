
#include "Python.h"

#include "pyio.h"

#include "eckit/config/Resource.h"
#include "eckit/exception/Exceptions.h"
#include "mir/util/Grib.h"

static size_t buffer_size() {
    static size_t size = eckit::Resource<size_t>("$MIR_GRIB_INPUT_BUFFER_SIZE", 64 * 1024 * 1024);
    return size;
}

static long pyio_readcb(void* data, void* buf, long len) {
    auto obj = reinterpret_cast<PyObject*>(data);
    PyObject* res = PyObject_CallMethod(obj, "read", "l", len);
    if (res == nullptr)
        return -1;

    Py_buffer read;
    if (PyObject_GetBuffer(res, &read, PyBUF_SIMPLE) < 0) {
        Py_DECREF(res);
        return -1;
    }

    Py_DECREF(res);

    long l = read.len;
    ASSERT(l <= len);
    if (PyBuffer_ToContiguous(buf, &read, l, 'C') < 0) {
        PyBuffer_Release(&read);
        return -1;
    }

    PyBuffer_Release(&read);
    return l;
}

GribPyIOInput::GribPyIOInput(PyObject* obj) : obj_(obj), buffer_(buffer_size()) {
    ASSERT(obj_ != nullptr);
    Py_INCREF(obj_);
}

GribPyIOInput::~GribPyIOInput() {
    ASSERT(obj_ != nullptr);
    Py_DECREF(obj_);
}

bool GribPyIOInput::next() {
    ASSERT(obj_ != nullptr);

    handle(nullptr);

    size_t len = buffer_.size();
    int e = wmo_read_any_from_stream(obj_, &pyio_readcb, buffer_, &len);

    if (e == CODES_SUCCESS) {
        ASSERT(handle(codes_handle_new_from_message(nullptr, buffer_, len)));
        return true;
    }

    if (e == CODES_END_OF_FILE) {
        return false;
    }

    if (e == CODES_BUFFER_TOO_SMALL) {
        GRIB_ERROR(e, "wmo_read_any_from_stream");
    }

    GRIB_ERROR(e, "wmo_read_any_from_stream");

    return false;
}

bool GribPyIOInput::sameAs(const mir::input::MIRInput& other) const {
    return this == &other;
}

void GribPyIOInput::print(std::ostream& out) const {
    out << "GribPyIOInput[]";
}
