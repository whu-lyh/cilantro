#pragma once

#include <cilantro/space_transformations.hpp>
#include <cilantro/common_pair_evaluators.hpp>
#include <cilantro/kd_tree.hpp>

namespace cilantro {
    template <class TransformT, class WeightEvaluatorT = UnityWeightEvaluator<typename TransformT::Scalar,typename TransformT::Scalar>>
    typename std::enable_if<internal::HasLinear<TransformT>::value && internal::HasTranslation<TransformT>::value,void>::type
    resampleTransforms(const TransformSet<TransformT> &old_transforms,
                       const NeighborhoodSet<typename WeightEvaluatorT::InputScalar> &new_to_old_map,
                       TransformSet<TransformT> &new_transforms,
                       const WeightEvaluatorT &weight_evaluator = WeightEvaluatorT())
    {
        new_transforms.resize(new_to_old_map.size());

#pragma omp parallel for shared (new_transforms)
        for (size_t i = 0; i < new_transforms.size(); i++) {
            typename TransformT::Scalar total_weight = (typename TransformT::Scalar)0.0;
            new_transforms[i].linear().setZero();
            new_transforms[i].translation().setZero();
            for (size_t j = 0; j < new_to_old_map[i].size(); j++) {
                const typename TransformT::Scalar weight = weight_evaluator(i, new_to_old_map[i][j].index, new_to_old_map[i][j].value);
                total_weight += weight;
                new_transforms[i].linear().noalias() += weight*old_transforms[new_to_old_map[i][j].index].linear();
                new_transforms[i].translation().noalias() += weight*old_transforms[new_to_old_map[i][j].index].translation();
            }

            if (total_weight == (typename TransformT::Scalar)0.0) {
                new_transforms[i].setIdentity();
            } else {
                total_weight = (typename TransformT::Scalar)(1.0)/total_weight;
                new_transforms[i].linear() *= total_weight;
                if (int(TransformT::Mode) == int(Eigen::Isometry)) {
                    new_transforms[i].linear() = new_transforms[i].rotation();
                }
                new_transforms[i].translation() *= total_weight;
            }
        }
    }

    template <class TransformT, class WeightEvaluatorT = UnityWeightEvaluator<typename TransformT::Scalar,typename TransformT::Scalar>>
    inline TransformSet<TransformT> resampleTransforms(const TransformSet<TransformT> &old_transforms,
                                                       const NeighborhoodSet<typename WeightEvaluatorT::InputScalar> &new_to_old_map,
                                                       const WeightEvaluatorT &weight_evaluator = WeightEvaluatorT())
    {
        TransformSet<TransformT> new_transforms;
        resampleTransforms<TransformT,WeightEvaluatorT>(old_transforms, new_to_old_map, new_transforms, weight_evaluator);
        return new_transforms;
    }

    template <class TransformT, class NeighborhoodSpecT, class WeightEvaluatorT = UnityWeightEvaluator<typename TransformT::Scalar,typename TransformT::Scalar>>
    typename std::enable_if<internal::HasLinear<TransformT>::value && internal::HasTranslation<TransformT>::value,void>::type
    resampleTransforms(const KDTree<typename TransformT::Scalar,TransformT::Dim,KDTreeDistanceAdaptors::L2> &old_support_kd_tree,
                       const TransformSet<TransformT> &old_transforms,
                       const ConstVectorSetMatrixMap<typename TransformT::Scalar,TransformT::Dim> &new_support,
                       const NeighborhoodSpecT &nh,
                       TransformSet<TransformT> &new_transforms,
                       const WeightEvaluatorT &weight_evaluator = WeightEvaluatorT())
    {
        new_transforms.resize(new_support.cols());

        Neighborhood<typename TransformT::Scalar> nn;
#pragma omp parallel for shared (new_transforms) private (nn)
        for (size_t i = 0; i < new_transforms.size(); i++) {
            old_support_kd_tree.template search<NeighborhoodSpecT>(new_support.col(i), nh, nn);
            typename TransformT::Scalar total_weight = (typename TransformT::Scalar)0.0;
            new_transforms[i].linear().setZero();
            new_transforms[i].translation().setZero();
            for (size_t j = 0; j < nn.size(); j++) {
                const typename TransformT::Scalar weight = weight_evaluator(i, nn[j].index, nn[j].value);
                total_weight += weight;
                new_transforms[i].linear().noalias() += weight*old_transforms[nn[j].index].linear();
                new_transforms[i].translation().noalias() += weight*old_transforms[nn[j].index].translation();
            }

            if (total_weight == (typename TransformT::Scalar)0.0) {
                new_transforms[i].setIdentity();
            } else {
                total_weight = (typename TransformT::Scalar)(1.0)/total_weight;
                new_transforms[i].linear() *= total_weight;
                if (int(TransformT::Mode) == int(Eigen::Isometry)) {
                    new_transforms[i].linear() = new_transforms[i].rotation();
                }
                new_transforms[i].translation() *= total_weight;
            }
        }
    }

    template <class TransformT, class NeighborhoodSpecT, class WeightEvaluatorT = UnityWeightEvaluator<typename TransformT::Scalar,typename TransformT::Scalar>>
    inline TransformSet<TransformT> resampleTransforms(const KDTree<typename TransformT::Scalar,TransformT::Dim,KDTreeDistanceAdaptors::L2> &old_support_kd_tree,
                                                       const TransformSet<TransformT> &old_transforms,
                                                       const ConstVectorSetMatrixMap<typename TransformT::Scalar,TransformT::Dim> &new_support,
                                                       const NeighborhoodSpecT &nh,
                                                       const WeightEvaluatorT &weight_evaluator = WeightEvaluatorT())
    {
        TransformSet<TransformT> new_transforms;
        resampleTransforms<TransformT,NeighborhoodSpecT,WeightEvaluatorT>(old_support_kd_tree, old_transforms, new_support, nh, new_transforms, weight_evaluator);
        return new_transforms;
    }
}
