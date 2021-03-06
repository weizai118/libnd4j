//
// This file contains operations added by 3rd parties
//
// @author raver119@gmail.com
//

#include <op_boilerplate.h>
#if NOT_EXCLUDED(OP_firas_sparse)

#ifndef LIBND4J_THIRD_PARTY_H
#define LIBND4J_THIRD_PARTY_H

#include <op_boilerplate.h>
#include <memory>
#include <shape.h>
#include <loops/random.h>
#include <NDArray.h>
#include <ops/declarable/DeclarableOp.h>
#include <ops/declarable/OpRegistrator.h>
#include <ops/declarable/CustomOperations.h>
#include <NDArrayFactory.h>

namespace nd4j {
    namespace ops {


        /**
         * This op is special one, and suited only for ProjectionLayer by @firasdib
         *
         *
         *
         * @tparam T
         */
        CUSTOM_OP_IMPL(firas_sparse, 1, 1, false, 0, -1) {
            auto x = INPUT_VARIABLE(0);
            auto z = OUTPUT_VARIABLE(0);

            int batchSize = x->sizeAt(0);
            int numColumns = x->sizeAt(1);

            std::vector<int> indices(*block.getIArguments());
            std::map<int, int> sparse2dense;


            int cnt = 0;
            for (auto v: indices) {
                std::pair<int, int> pair(v, cnt++);
                sparse2dense.insert(pair);
            }

            std::unique_ptr<ResultSet<T>> rows(NDArrayFactory<T>::allTensorsAlongDimension(x, {1}));

#pragma omp parallel for schedule(dynamic) proc_bind(close)
            for (int r = 0; r < batchSize; r++) {
                auto row = rows->at(r);

                for (int e = 0; e < numColumns; e += 2) {
                    int idx = row->getIndexedScalar(e);
                    if (idx < 0)
                        break;

                    int denseIdx = sparse2dense.at(idx);


                    T value = row->getIndexedScalar(e + 1);
                    T current = z->getScalar(r, denseIdx);
                    z->putScalar(r, denseIdx, value + current);
                }
            }


            STORE_RESULT(*z);

            return ND4J_STATUS_OK;
        }

        DECLARE_SHAPE_FN(firas_sparse) {
            auto inP = inputShape->at(0);

            Nd4jLong *newShape;
            ALLOCATE(newShape, block.getWorkspace(), shape::shapeInfoLength(2), Nd4jLong);

            std::vector<Nd4jLong> shape({shape::shapeOf(inP)[0], (Nd4jLong) block.getIArguments()->size()});
            shape::shapeBuffer(2, shape.data(), newShape);

            return SHAPELIST(newShape);
        }
    }
}

#endif

#endif //LIBND4J_THIRD_PARTY_H
