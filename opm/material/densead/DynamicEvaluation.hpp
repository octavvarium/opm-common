// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \brief This file file provides a dense-AD Evaluation class where the
 *        number of derivatives is specified at runtime.
 *
 * \attention THIS FILE GETS AUTOMATICALLY GENERATED BY THE "genEvalSpecializations.py"
 *            SCRIPT. DO NOT EDIT IT MANUALLY!
 */
#ifndef OPM_DENSEAD_EVALUATION_DYNAMIC_HPP
#define OPM_DENSEAD_EVALUATION_DYNAMIC_HPP

#include "Evaluation.hpp"
#include "Math.hpp"

#include <opm/material/common/Valgrind.hpp>

#include <opm/material/common/FastSmallVector.hpp>
#include <cmath>
#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace Opm {
namespace DenseAd {

/*!
 * \brief Represents a function evaluation and its derivatives w.r.t. a
 *        run-time specified set of variables.
 */
template <class ValueT, unsigned staticSize>
class Evaluation<ValueT, DynamicSize, staticSize>
{
public:
    //! the template argument which specifies the number of
    //! derivatives (-1 == "DynamicSize" means runtime determined)
    static const int numVars = DynamicSize;

    //! field type
    typedef ValueT ValueType;

    //! number of derivatives
    int size() const
    { return data_.size() - 1; }

protected:
    //! length of internal data vector
    int length_() const
    { return data_.size(); }


    //! position index for value
    constexpr int valuepos_() const
    { return 0; }
    //! start index for derivatives
    constexpr int dstart_() const
    { return 1; }
    //! end+1 index for derivatives
    int dend_() const
    { return length_(); }

    //! instruct valgrind to check that the value and all derivatives of the
    //! Evaluation object are well-defined.
    void checkDefined_() const
    {
#ifndef NDEBUG
        for (int i = dstart_(); i < dend_(); ++i)
            Valgrind::CheckDefined(data_[i]);
#endif
    }

public:
    //! default constructor
    Evaluation() : data_()
    {}

    //! copy other function evaluation
    Evaluation(const Evaluation& other) = default;

    //! move other function evaluation (this only makes sense for dynamically
    //! allocated Evaluations)
    Evaluation(Evaluation&& other)
        : data_(std::move(other.data_))
    { }

    //! move assignment
    Evaluation& operator=(Evaluation&& other)
    {
        data_ = std::move(other.data_);
        return *this;
    }

    // create a "blank" dynamic evaluation
    explicit Evaluation(int numDerivatives)
        : data_(1 + numDerivatives)
    {}

    // create a dynamic evaluation which represents a constant function
    //
    // i.e., f(x) = c. this implies an evaluation with the given value and all
    // derivatives being zero.
    template <class RhsValueType>
    Evaluation(int numDerivatives, const RhsValueType& c)
        : data_(1 + numDerivatives, 0.0)
    {
        //clearDerivatives();
        setValue(c);

        checkDefined_();
    }

    // create an evaluation which represents a constant function
    //
    // i.e., f(x) = c. this implies an evaluation with the given value and all
    // derivatives being zero.
    template <class RhsValueType>
    Evaluation(int nVars, const RhsValueType& c, int varPos)
     : data_(1 + nVars, 0.0)
    {
        // The variable position must be in represented by the given variable descriptor
        assert(0 <= varPos && varPos < size());

        setValue(c);

        data_[varPos + dstart_()] = 1.0;

        checkDefined_();
    }

    // set all derivatives to zero
    void clearDerivatives()
    {
        for (int i = dstart_(); i < dend_(); ++i)
            data_[i] = 0.0;
    }

    // create an uninitialized Evaluation object that is compatible with the
    // argument, but not initialized
    //
    // This basically boils down to the copy constructor without copying
    // anything. If the number of derivatives is known at compile time, this
    // is equivalent to creating an uninitialized object using the default
    // constructor, while for dynamic evaluations, it creates an Evaluation
    // object which exhibits the same number of derivatives as the argument.
    static Evaluation createBlank(const Evaluation& x)
    { return Evaluation(x.size()); }

    // create a function evaluation for a "naked" depending variable (i.e., f(x) = x)
    template <class RhsValueType>
    static Evaluation createVariable(int nVars, const RhsValueType& value, int varPos)
    {
        // copy function value and set all derivatives to 0, except for the variable
        // which is represented by the value (which is set to 1.0)
        return Evaluation(nVars, value, varPos);
    }


    // "evaluate" a constant function (i.e. a function that does not depend on the set of
    // relevant variables, f(x) = c).
    template <class RhsValueType>
    static Evaluation createConstant(int nVars, const RhsValueType& value)
    {
        return Evaluation(nVars, value);
    }

    // print the value and the derivatives of the function evaluation
    void print(std::ostream& os = std::cout) const
    {
        // print value
        os << "v: " << value() << " / d:";

        // print derivatives
        for (int varIdx = 0; varIdx < size(); ++varIdx) {
            os << " " << derivative(varIdx);
        }
    }

    // copy all derivatives from other
    void copyDerivatives(const Evaluation& other)
    {
        assert(size() == other.size());

        for (int i = dstart_(); i < dend_(); ++i)
            data_[i] = other.data_[i];
    }


    // add value and derivatives from other to this values and derivatives
    Evaluation& operator+=(const Evaluation& other)
    {
        assert(size() == other.size());

        for (int i = 0; i < length_(); ++i)
            data_[i] += other.data_[i];

        return *this;
    }

    // add value from other to this values
    template <class RhsValueType>
    Evaluation& operator+=(const RhsValueType& other)
    {
        // value is added, derivatives stay the same
        data_[valuepos_()] += other;

        return *this;
    }

    // subtract other's value and derivatives from this values
    Evaluation& operator-=(const Evaluation& other)
    {
        assert(size() == other.size());

        for (int i = 0; i < length_(); ++i)
            data_[i] -= other.data_[i];

        return *this;
    }

    // subtract other's value from this values
    template <class RhsValueType>
    Evaluation& operator-=(const RhsValueType& other)
    {
        // for constants, values are subtracted, derivatives stay the same
        data_[valuepos_()] -= other;

        return *this;
    }

    // multiply values and apply chain rule to derivatives: (u*v)' = (v'u + u'v)
    Evaluation& operator*=(const Evaluation& other)
    {
        assert(size() == other.size());

        // while the values are multiplied, the derivatives follow the product rule,
        // i.e., (u*v)' = (v'u + u'v).
        const ValueType u = this->value();
        const ValueType v = other.value();

        // value
        data_[valuepos_()] *= v ;

        //  derivatives
        for (int i = dstart_(); i < dend_(); ++i)
            data_[i] = data_[i] * v + other.data_[i] * u;

        return *this;
    }

    // m(c*u)' = c*u'
    template <class RhsValueType>
    Evaluation& operator*=(const RhsValueType& other)
    {
        for (int i = 0; i < length_(); ++i)
            data_[i] *= other;

        return *this;
    }

    // m(u*v)' = (vu' - uv')/v^2
    Evaluation& operator/=(const Evaluation& other)
    {
        assert(size() == other.size());

        // values are divided, derivatives follow the rule for division, i.e., (u/v)' = (v'u -
        // u'v)/v^2.
        ValueType& u = data_[valuepos_()];
        const ValueType& v = other.value();
        for (int idx = dstart_(); idx < dend_(); ++idx) {
            const ValueType& uPrime = data_[idx];
            const ValueType& vPrime = other.data_[idx];

            data_[idx] = (v*uPrime - u*vPrime)/(v*v);
        }
        u /= v;

        return *this;
    }

    // divide value and derivatives by value of other
    template <class RhsValueType>
    Evaluation& operator/=(const RhsValueType& other)
    {
        const ValueType tmp = 1.0/other;

        for (int i = 0; i < length_(); ++i)
            data_[i] *= tmp;

        return *this;
    }

    // add two evaluation objects
    Evaluation operator+(const Evaluation& other) const
    {
        assert(size() == other.size());

        Evaluation result(*this);

        result += other;

        return result;
    }

    // add constant to this object
    template <class RhsValueType>
    Evaluation operator+(const RhsValueType& other) const
    {
        Evaluation result(*this);

        result += other;

        return result;
    }

    // subtract two evaluation objects
    Evaluation operator-(const Evaluation& other) const
    {
        assert(size() == other.size());

        Evaluation result(*this);

        result -= other;

        return result;
    }

    // subtract constant from evaluation object
    template <class RhsValueType>
    Evaluation operator-(const RhsValueType& other) const
    {
        Evaluation result(*this);

        result -= other;

        return result;
    }

    // negation (unary minus) operator
    Evaluation operator-() const
    {
        Evaluation result(*this);

        // set value and derivatives to negative
        for (int i = 0; i < length_(); ++i)
            result.data_[i] = - data_[i];

        return result;
    }

    Evaluation operator*(const Evaluation& other) const
    {
        assert(size() == other.size());

        Evaluation result(*this);

        result *= other;

        return result;
    }

    template <class RhsValueType>
    Evaluation operator*(const RhsValueType& other) const
    {
        Evaluation result(*this);

        result *= other;

        return result;
    }

    Evaluation operator/(const Evaluation& other) const
    {
        assert(size() == other.size());

        Evaluation result(*this);

        result /= other;

        return result;
    }

    template <class RhsValueType>
    Evaluation operator/(const RhsValueType& other) const
    {
        Evaluation result(*this);

        result /= other;

        return result;
    }

    template <class RhsValueType>
    Evaluation& operator=(const RhsValueType& other)
    {
        setValue( other );
        clearDerivatives();

        return *this;
    }

    // copy assignment from evaluation
    Evaluation& operator=(const Evaluation& other) = default;

    template <class RhsValueType>
    bool operator==(const RhsValueType& other) const
    { return value() == other; }

    bool operator==(const Evaluation& other) const
    {
        assert(size() == other.size());

        for (int idx = 0; idx < length_(); ++idx) {
            if (data_[idx] != other.data_[idx]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Evaluation& other) const
    { return !operator==(other); }

    template <class RhsValueType>
    bool operator!=(const RhsValueType& other) const
    { return !operator==(other); }

    template <class RhsValueType>
    bool operator>(RhsValueType other) const
    { return value() > other; }

    bool operator>(const Evaluation& other) const
    {
        assert(size() == other.size());

        return value() > other.value();
    }

    template <class RhsValueType>
    bool operator<(RhsValueType other) const
    { return value() < other; }

    bool operator<(const Evaluation& other) const
    {
        assert(size() == other.size());

        return value() < other.value();
    }

    template <class RhsValueType>
    bool operator>=(RhsValueType other) const
    { return value() >= other; }

    bool operator>=(const Evaluation& other) const
    {
        assert(size() == other.size());

        return value() >= other.value();
    }

    template <class RhsValueType>
    bool operator<=(RhsValueType other) const
    { return value() <= other; }

    bool operator<=(const Evaluation& other) const
    {
        assert(size() == other.size());

        return value() <= other.value();
    }

    // return value of variable
    const ValueType& value() const
    { return data_[valuepos_()]; }

    // set value of variable
    template <class RhsValueType>
    void setValue(const RhsValueType& val)
    { data_[valuepos_()] = val; }

    // return varIdx'th derivative
    const ValueType& derivative(int varIdx) const
    {
        assert(0 <= varIdx && varIdx < size());

        return data_[dstart_() + varIdx];
    }

    // set derivative at position varIdx
    void setDerivative(int varIdx, const ValueType& derVal)
    {
        assert(0 <= varIdx && varIdx < size());

        data_[dstart_() + varIdx] = derVal;
    }

private:

    FastSmallVector<ValueT, staticSize> data_;
};

template <class Scalar, unsigned staticSize = 9>
using DynamicEvaluation = Evaluation<Scalar, DynamicSize, staticSize>;

} // namespace DenseAd

template <class Scalar, unsigned staticSize>
Opm::DenseAd::Evaluation<Scalar, -1, staticSize> constant(int numDerivatives, const Scalar& value)
{ return Opm::DenseAd::Evaluation<Scalar, -1, staticSize>::createConstant(numDerivatives, value); }

template <class Scalar, unsigned staticSize>
Opm::DenseAd::Evaluation<Scalar, -1, staticSize> variable(int numDerivatives, const Scalar& value, unsigned idx)
{ return Opm::DenseAd::Evaluation<Scalar, -1, staticSize>::createVariable(numDerivatives, value, idx); }

} // namespace Opm

#endif // OPM_DENSEAD_EALUATION-1_HPP
