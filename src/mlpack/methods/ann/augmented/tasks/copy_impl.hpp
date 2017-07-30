/**
 * @file copy_impl.hpp
 * @author Konstantin Sidorov
 *
 * Implementation of CopyTask class
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_METHODS_AUGMENTED_TASKS_COPY_IMPL_HPP
#define MLPACK_METHODS_AUGMENTED_TASKS_COPY_IMPL_HPP

// In case it hasn't been included yet.
#include "copy.hpp"

namespace mlpack {
namespace ann /* Artificial Neural Network */ {
namespace augmented /* Augmented neural network */ {
namespace tasks /* Task utilities for augmented */ {

CopyTask::CopyTask(const size_t maxLength,
                   const size_t nRepeats,
                   const bool addSeparator) :
    maxLength(maxLength),
    nRepeats(nRepeats),
    addSeparator(addSeparator)
{
  if (maxLength <= 1) 
  {
    std::ostringstream oss;
    oss << "CopyTask::CopyTask(): maximum sequence length ("
        << maxLength << ") "
        << "should be at least 2!"
        << std::endl;
    throw std::invalid_argument(oss.str());
  }
  if (nRepeats <= 0)
  {
    std::ostringstream oss;
    oss << "CopyTask::CopyTask(): repetition count (" << nRepeats << ") "
        << "is not positive!"
        << std::endl;
    throw std::invalid_argument(oss.str());
  }
  // Just storing task-specific parameters.
}

const void CopyTask::Generate(arma::field<arma::mat>& input,
                              arma::field<arma::mat>& labels,
                              const size_t batchSize,
                              bool fixedLength)
{
  input = arma::field<arma::mat>(batchSize);
  labels = arma::field<arma::mat>(batchSize);
  size_t size = maxLength;
  for (size_t i = 0; i < batchSize; ++i)
  {
    if (!fixedLength)
    {      
      std::vector<double> weights(maxLength - 1);
      // We have four binary sequences with exactly two digits.
      weights[0] = 2;
      // Increasing length by 1 double the number of valid numbers.
      for (size_t i = 1; i < maxLength - 1; ++i)
        weights[i] = 2 * weights[i - 1];
      // Generate random uniform length from [2..maxLength].
      size = 2 + mlpack::math::RandInt(weights);
    }
    arma::colvec vecInput = arma::randi<arma::colvec>(
      size, arma::distr_param(0, 1));
    arma::colvec vecLabel = arma::conv_to<arma::colvec>::from(
      arma::repmat(vecInput, nRepeats, 1));
    size_t totSize = vecInput.n_elem + addSeparator + vecLabel.n_elem;
    input(i) = arma::zeros(totSize, 2);
    input(i).col(0).rows(0, vecInput.n_elem-1) =
      vecInput;
    if (addSeparator)
      input(i).at(vecInput.n_elem, 0) = 0.5;
    input(i).col(1).rows(addSeparator + vecInput.n_elem, totSize - 1) =
        arma::ones(totSize-vecInput.n_elem - addSeparator);
    input(i) = input(i).t();
    input(i).reshape(input(i).n_elem, 1);
    labels(i) = arma::zeros(totSize, 1);
    labels(i).col(0).rows(addSeparator + vecInput.n_elem, totSize - 1) =
        vecLabel;
  }
}

const void CopyTask::Generate(arma::mat& input,
                              arma::mat& labels,
                              const size_t batchSize)
{
  arma::field<arma::mat> fieldInput, fieldLabels;
  Generate(fieldInput, fieldLabels, batchSize, true);
  size_t cols = batchSize;
  input = arma::zeros(fieldInput(0).n_rows, cols);
  labels = arma::zeros(fieldLabels(0).n_rows, cols);
  for (size_t i = 0; i < cols; ++i)
  {
    input.col(i) = fieldInput.at(i);
    labels.col(i) = fieldLabels.at(i);
  }
}


} // namespace tasks
} // namespace augmented
} // namespace ann
} // namespace mlpack

#endif
