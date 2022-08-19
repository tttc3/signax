#include <iostream>
#include <torch/torch.h>
#include "pybind11_helpers.h"
#include "signatory/signature.hpp"
#include <torch/extension.h>  // to get the pybind11 stuff

namespace signax {
    template<typename T>
    void cpuSignatureForward(void *out, void **in) {
        // DATA TRANSFORMATION
        int shapeLength = 3;
        auto shape = reinterpret_cast<long *>(in[1]);

        auto pathSource = reinterpret_cast<T *>(in[0]);
        auto pathShapeRef = new torch::IntArrayRef(shape, shapeLength);

        auto options = torch::TensorOptions().dtype(torch::CppTypeToScalarType<T>());
        torch::Tensor path = torch::from_blob(pathSource, *pathShapeRef, options);

        std::cout << "cpu_ops.cc::cpuSignatureForward\n";
        std::cout << "Path: \n" << path << '\n';

        int depth = (long int) *reinterpret_cast<int *>(in[2]);
        std::cout << "Depth: \n" << depth << '\n';

        auto *emptyTensor = new torch::Tensor();

        signatory::signature_forward(
                /*torch::Tensor*/ path,
                /*s_size_type*/ depth,
                /*bool stream*/ false,
                /*bool basepoint*/ false,
                /*torch::Tensor basepoint_value*/ *emptyTensor,
                /*bool inverse*/ false,
                /*bool initial*/ false,
                /*torch::Tensor initial_value*/ *emptyTensor,
                /*bool scalar_term*/ false);

        // DATA RECOVERY
        long flattenedPath = shape[0] * shape[1] * shape[2];
        T (*ans)[flattenedPath] = reinterpret_cast<T(*)[flattenedPath]>(out);
        T *recoveredData = path.data_ptr<T>();

        for (int i = 0; i < flattenedPath; ++i)
            (*ans)[i] = (*recoveredData++);
    }

    pybind11::dict Registrations() {
        pybind11::dict dict;
        dict["cpu_signature_forward_f32"] = EncapsulateFunction(cpuSignatureForward<float>);
        dict["cpu_signature_forward_f64"] = EncapsulateFunction(cpuSignatureForward<double>);
        return dict;
    }

    PYBIND11_MODULE(TORCH_EXTENSION_NAME, m
    ) {
    m.def("cpu_registrations", &Registrations);
}
}
