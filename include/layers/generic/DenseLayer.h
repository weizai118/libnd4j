//
// @author raver119@gmail.com
//

#ifndef PROJECT_DENSE_H
#define PROJECT_DENSE_H

#include <layers/layers.h>
#include <layers/generic/BaseLayer.h>

namespace nd4j {
namespace layers {

template<typename T, typename AF> class DenseLayer: public BaseLayer<T, AF> {
    public:

        // default constructor
        DenseLayer();     

        // feed forward
        int feedForward();

        // back propagate
        int backPropagate();
       
        // This method should validate layer parameters & bias, and return TRUE if everything ok. FALSE otherwise      
        inline int validateParameters();

        // This method should validate input parameters, and return TRUE if everything ok. FALSE otherwise
        inline int validateInput();

        // This method should valudate output parameters, and return TRUE if everything is ok, FALSE otherwise        
        inline int validateOutput();
};





/////// implementation part ///////    

// default constructor
template<typename T, typename AF> DenseLayer<T,AF>::DenseLayer() { 

}     

// back propagate
template<typename T, typename AF> int DenseLayer<T,AF>::backPropagate() {
    // delta = dL/dz
    // epsilon = dL/da
    // delta = epsilon * da/dz = next_params_T * next_delta (*) da/dz
    T* delta = new T[sizeof(T) * shape::length(this->epsilontShapeInfo)];
    ActivationsExecutioner<T>::template executeBP<AF>(this->input, this->epsilon, this->delta, this->inputShapeInfo);
    // gradient_on_param = delta * previous_output
    // gradient_on_bias = delta
    this->gemmHelper(this->input, this->inputShapeInfo, this->params, this->paramsShapeInfo, this->output, this->outputShapeInfo, (T) 1.0f, (T) 0.0f);

    //this->gemmHelper(this->input, this->inputShapeInfo, delta, this->epsilonShapeInfo, this->output, this->outputShapeInfo, (T)1.0f, (T)0.0f);
    
    // INDArray delta = layerConf().getActivationFn().backprop(z, epsilon).getFirst();
    // how to evaluate delta, what is it ???
    // this->gemmHelper(this->input, this->inputShapeInfo, this->params, this->paramsShapeInfo, this->output, this->outputShapeInfo, (T) 1.0f, (T) 0.0f);
    // Nd4j.gemm(input, delta, weightGrad, true, false, 1.0, 0.0);
    delete [] delta;
    return ND4J_STATUS_OK;
}







  // inline static T bpActivation(T value, T epsilon) {
                // // FIXME: ultra-bad. should consider conigurable extra params here
                // T extra[] = {(T) 0.0f};
                // return simdOps::Step<T>::template op(value, extra) * epsilon;





// This method should validate layer parameters & bias, and return TRUE if everything ok. FALSE otherwise
template<typename T, typename AF>
int DenseLayer<T,AF>::validateParameters() {
    if (this->params->shapeInfo == nullptr || this->bias->shapeInfo == nullptr || this->params == nullptr || this->bias == nullptr || this->params->buffer == nullptr || this->bias->buffer == nullptr) {
//        printf("Got nulls here\n");
        return ND4J_STATUS_BAD_PARAMS;
    }

    int wRank = this->params->rankOf();
    int bRank = this->bias->rankOf();

    // rank of params/bias has to be 2 here
    if (wRank != 2 || bRank != 2) {
//        printf("Non 2\n");
        return ND4J_STATUS_BAD_RANK;
    }


    int *wShape = this->params->shapeOf();

    int biasLength = this->bias->lengthOf();

    // number of outputs must be equal to biasLength
    if (wShape[1] != biasLength) {
//        printf("Bias doesn't match: %i vs %i\n", wShape[1], biasLength);
        return ND4J_STATUS_BAD_SHAPE;
    }


    return ND4J_STATUS_OK;
}


// This method should validate input parameters, and return TRUE if everything ok. FALSE otherwise
template<typename T, typename AF> int DenseLayer<T,AF>::validateInput() {
    // we expect input to be either vector or matrix, in both cases - that's rank2
    if (this->input == nullptr || this->input->shapeInfo == nullptr ||this->input->buffer == nullptr)
        return ND4J_STATUS_BAD_INPUT;

    if (this->input->rankOf() != 2)
        return ND4J_STATUS_BAD_RANK;


    int *iShape = this->input->shapeOf();

    if (this->params != nullptr && this->params->nonNull()) {
        // check dimensionality

        int *wShape = this->params->shapeOf();

        // number of input features should match number of rows in params
        if (iShape[1] != wShape[0]) {
            return ND4J_STATUS_BAD_SHAPE;
        }
    }

    if (this->output != nullptr && this->output->nonNull()) {
        int *oShape = this->output->shapeOf();

        // we check for input/output batchSize equality
        if (oShape[0] != iShape[0])
            return ND4J_STATUS_BAD_SHAPE;
    }

    return ND4J_STATUS_OK;
}


// This method should valudate output parameters, and return TRUE if everything is ok, FALSE otherwise
template<typename T, typename AF> int DenseLayer<T,AF>::validateOutput() {
    // same as input validation here. we expect rank of output arra
    if (this->output == nullptr || this->output->buffer == nullptr || this->output->shapeInfo == nullptr)
        return ND4J_STATUS_BAD_OUTPUT;

    if (this->output->rankOf() != 2)
        return ND4J_STATUS_BAD_RANK;

    int *oShape = this->output->shapeOf();

    // length of output along dimension 1 should match length of parameters, if parameters are set,
    if (this->params != nullptr && this->params->nonNull()) {
        int *wShape = this->params->shapeOf();

        // number of output features should match number of rows in params
        if (oShape[1] != wShape[1]) {
            return ND4J_STATUS_BAD_SHAPE;
        }
    }


    if (this->input != nullptr && this->input->nonNull()) {
        int *iShape = this->input->shapeOf();

        // we check for input/output batchSize equality
        if (oShape[0] != iShape[0])
            return ND4J_STATUS_BAD_SHAPE;
    }

    return ND4J_STATUS_OK;
}

// feed forward
template<typename T, typename AF> int DenseLayer<T,AF>::feedForward() {
    // dropout helper call
    if (this->dropOut) {
        //printf("Going dropout\n");
        this->dropOutHelper(this->input);
    }

    // dropconnect helper
    if (this->dropConnect) {
        //printf("Going dropconnect\n");
        this->dropConnectHelper(this->params);
    }
    

    // do wxa+b here or something else
    // TODO: introduce BLAS right here
    if (shape::isRowVector(this->input->shapeInfo)) {
        // gemv here input * W

    } else {
        // gemm here, input * W
        // these values should be set appropriately

        this->gemmHelper(this->input, this->params, this->output, (T) 1.0f, (T) 0.0f);

        // we're rolling through rows here
        this->output->addiRowVector(this->bias);
    }

    // activation call
    ActivationsExecutioner<T>::template executeFF<AF>(this->output, this->output);

    return ND4J_STATUS_OK;
}


// end of namespace brackets
}
}

#endif //PROJECT_DENSE_H