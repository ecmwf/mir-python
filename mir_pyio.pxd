
cimport mir_defs as mir

cdef extern from "pyio.h":
    cdef cppclass GribPyIOInput(mir.MIRInput):
        GribPyIOInput(object)
