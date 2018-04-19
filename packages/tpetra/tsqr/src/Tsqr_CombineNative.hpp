//@HEADER
// ************************************************************************
//
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
//@HEADER

/// \file Tsqr_CombineNative.hpp
/// \brief Interface to C++ back end of \c TSQR::Combine.
///
#ifndef __TSQR_CombineNative_hpp
#define __TSQR_CombineNative_hpp

#include <Teuchos_BLAS.hpp>
#include <Teuchos_LAPACK.hpp>
#include <Teuchos_ScalarTraits.hpp>

#include "Tsqr_ApplyType.hpp"
#include "Tsqr_CombineDefault.hpp"

#include "Kokkos_Core.hpp"
#include "KokkosBlas2_gemv.hpp"

namespace TSQR {

  /// \class CombineNative
  /// \brief Interface to C++ back end of TSQR::Combine
  ///
  /// \c TSQR::Combine has three implementations: \c CombineDefault,
  /// CombineNative, and \c CombineFortran.  CombineNative,
  /// implemented in this file, is a fully C++ (therefore "native," as
  /// opposed to \c CombineFortran (implemented in Fortran) or \c
  /// CombineNative (implemented by wrappers around LAPACK calls))
  /// implementation.
  ///
  /// \warning CombineNative has no complex-arithmetic implementation
  ///   yet.  It's not hard to implement this (use LAPACK's ZGEQR2(P)
  ///   and ZUNM2R as models), but it will take time that the author
  ///   doesn't have at the moment.
  ///
  template< class Ordinal, class Scalar, bool isComplex = Teuchos::ScalarTraits< Scalar >::isComplex >
  class CombineNative
  {
  public:
    typedef Scalar scalar_type;
    typedef typename Teuchos::ScalarTraits< Scalar >::magnitudeType magnitude_type;
    typedef Ordinal ordinal_type;

  private:
    typedef Teuchos::BLAS<ordinal_type, scalar_type> blas_type;
    typedef Teuchos::LAPACK<ordinal_type, scalar_type> lapack_type;
    typedef CombineDefault<ordinal_type, scalar_type> combine_default_type;

  public:

    CombineNative () {}

    /// Whether or not the QR factorizations computed by methods of
    /// this class produce an R factor with all nonnegative diagonal
    /// entries.  It depends on LAPACK because this implementation
    /// invokes one of {LARFGP, LARFP, LARFG} in order to compute
    /// Householder reflectors; only LAPACK versions >= 3.2 have one
    /// of {LARFGP, LARFP}, which is necessary to ensure that the BETA
    /// output of the function is always nonnegative.
    static bool QR_produces_R_factor_with_nonnegative_diagonal() {
      return /* lapack_type::QR_produces_R_factor_with_nonnegative_diagonal() */ false &&
        combine_default_type::QR_produces_R_factor_with_nonnegative_diagonal();
    }

    void
    factor_first (const Ordinal nrows,
                  const Ordinal ncols,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const
    {
      return default_.factor_first (nrows, ncols, A, lda, tau, work);
    }

    void
    apply_first (const ApplyType& applyType,
                 const Ordinal nrows,
                 const Ordinal ncols_C,
                 const Ordinal ncols_A,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C[],
                 const Ordinal ldc,
                 Scalar work[]) const
    {
      return default_.apply_first (applyType, nrows, ncols_C, ncols_A,
                                   A, lda, tau, C, ldc, work);
    }

    void
    apply_inner (const ApplyType& applyType,
                 const Ordinal m,
                 const Ordinal ncols_C,
                 const Ordinal ncols_Q,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C_top[],
                 const Ordinal ldc_top,
                 Scalar C_bot[],
                 const Ordinal ldc_bot,
                 Scalar work[]) const;

    void
    factor_inner (const Ordinal m,
                  const Ordinal n,
                  Scalar R[],
                  const Ordinal ldr,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const;

    void
    factor_pair (const Ordinal n,
                 Scalar R_top[],
                 const Ordinal ldr_top,
                 Scalar R_bot[],
                 const Ordinal ldr_bot,
                 Scalar tau[],
                 Scalar work[]) const;

    void
    apply_pair (const ApplyType& applyType,
                const Ordinal ncols_C,
                const Ordinal ncols_Q,
                const Scalar R_bot[],
                const Ordinal ldr_bot,
                const Scalar tau[],
                Scalar C_top[],
                const Ordinal ldc_top,
                Scalar C_bot[],
                const Ordinal ldc_bot,
                Scalar work[]) const;

  private:
    mutable combine_default_type default_;
  };


  //! Specialization of CombineNative for the real-arithmetic case.
  template< class Ordinal, class Scalar >
  class CombineNative< Ordinal, Scalar, false >
  {
  public:
    typedef Scalar scalar_type;
    typedef typename Teuchos::ScalarTraits< Scalar >::magnitudeType magnitude_type;
    typedef Ordinal ordinal_type;

  private:
    typedef Teuchos::BLAS<ordinal_type, scalar_type> blas_type;
    typedef Teuchos::LAPACK<ordinal_type, scalar_type> lapack_type;
    typedef CombineDefault<ordinal_type, scalar_type> combine_default_type;

    void
    GER (const magnitude_type alpha,
         const scalar_type x[],
         const Ordinal incx,
         const scalar_type y[],
         const Ordinal incy,
         const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& A) const;

    void
    GEMV (const char trans[],
          const scalar_type alpha,
          const Kokkos::View<const scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& A,
          const scalar_type x[],
          const Ordinal incx,
          const scalar_type beta,
          scalar_type y[],
          const Ordinal incy) const;

    void
    factor_pair (const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_top,
                 const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_bot,
                 scalar_type tau[],
                 scalar_type work[]) const;

    void
    apply_pair (const ApplyType& applyType,
                const Kokkos::View<const scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_bot, // ncols_Q
                const scalar_type tau[],
                const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& C_top, // ncols_C
                const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& C_bot,
                scalar_type work[]) const;

  public:
    CombineNative () {}

    static bool QR_produces_R_factor_with_nonnegative_diagonal() {
      return /* lapack_type::QR_produces_R_factor_with_nonnegative_diagonal() */ false &&
        combine_default_type::QR_produces_R_factor_with_nonnegative_diagonal();
    }

    void
    factor_first (const Ordinal nrows,
                  const Ordinal ncols,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const
    {
      return default_.factor_first (nrows, ncols, A, lda, tau, work);
    }

    void
    apply_first (const ApplyType& applyType,
                 const Ordinal nrows,
                 const Ordinal ncols_C,
                 const Ordinal ncols_A,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C[],
                 const Ordinal ldc,
                 Scalar work[]) const
    {
      return default_.apply_first (applyType, nrows, ncols_C, ncols_A,
                                   A, lda, tau, C, ldc, work);
    }

    void
    apply_inner (const ApplyType& applyType,
                 const Ordinal m,
                 const Ordinal ncols_C,
                 const Ordinal ncols_Q,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C_top[],
                 const Ordinal ldc_top,
                 Scalar C_bot[],
                 const Ordinal ldc_bot,
                 Scalar work[]) const;

    void
    factor_inner (const Ordinal m,
                  const Ordinal n,
                  Scalar R[],
                  const Ordinal ldr,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const;

    void
    factor_pair (const Ordinal n,
                 Scalar R_top[],
                 const Ordinal ldr_top,
                 Scalar R_bot[],
                 const Ordinal ldr_bot,
                 Scalar tau[],
                 Scalar work[]) const;

    void
    apply_pair (const ApplyType& applyType,
                const Ordinal ncols_C,
                const Ordinal ncols_Q,
                const scalar_type R_bot[],
                const Ordinal ldr_bot,
                const scalar_type tau[],
                scalar_type C_top[],
                const Ordinal ldc_top,
                scalar_type C_bot[],
                const Ordinal ldc_bot,
                scalar_type work[]) const;

  private:
    mutable combine_default_type default_;
  };


  /// "Forward declaration" for the complex-arithmetic case.
  ///
  template< class Ordinal, class Scalar >
  class CombineNative< Ordinal, Scalar, true >
  {
  public:
    typedef Scalar scalar_type;
    typedef typename Teuchos::ScalarTraits< Scalar >::magnitudeType magnitude_type;
    typedef Ordinal ordinal_type;

  private:
    typedef Teuchos::BLAS<ordinal_type, scalar_type> blas_type;
    typedef Teuchos::LAPACK<ordinal_type, scalar_type> lapack_type;
    typedef CombineDefault<ordinal_type, scalar_type> combine_default_type;

  public:
    CombineNative () {}

    static bool QR_produces_R_factor_with_nonnegative_diagonal() {
      return /* lapack_type::QR_produces_R_factor_with_nonnegative_diagonal() */ false &&
        combine_default_type::QR_produces_R_factor_with_nonnegative_diagonal();
    }

    void
    factor_first (const Ordinal nrows,
                  const Ordinal ncols,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const
    {
      return default_.factor_first (nrows, ncols, A, lda, tau, work);
    }

    void
    apply_first (const ApplyType& applyType,
                 const Ordinal nrows,
                 const Ordinal ncols_C,
                 const Ordinal ncols_A,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C[],
                 const Ordinal ldc,
                 Scalar work[]) const
    {
      return default_.apply_first (applyType, nrows, ncols_C, ncols_A,
                                   A, lda, tau, C, ldc, work);
    }

    void
    apply_inner (const ApplyType& applyType,
                 const Ordinal m,
                 const Ordinal ncols_C,
                 const Ordinal ncols_Q,
                 const Scalar A[],
                 const Ordinal lda,
                 const Scalar tau[],
                 Scalar C_top[],
                 const Ordinal ldc_top,
                 Scalar C_bot[],
                 const Ordinal ldc_bot,
                 Scalar work[]) const
    {
      return default_.apply_inner (applyType, m, ncols_C, ncols_Q,
                                   A, lda, tau,
                                   C_top, ldc_top, C_bot, ldc_bot,
                                   work);
    }

    void
    factor_inner (const Ordinal m,
                  const Ordinal n,
                  Scalar R[],
                  const Ordinal ldr,
                  Scalar A[],
                  const Ordinal lda,
                  Scalar tau[],
                  Scalar work[]) const
    {
      return default_.factor_inner (m, n, R, ldr, A, lda, tau, work);
    }

    void
    factor_pair (const Ordinal n,
                 Scalar R_top[],
                 const Ordinal ldr_top,
                 Scalar R_bot[],
                 const Ordinal ldr_bot,
                 Scalar tau[],
                 Scalar work[]) const
    {
      return default_.factor_pair (n, R_top, ldr_top, R_bot, ldr_bot, tau, work);
    }

    void
    apply_pair (const ApplyType& applyType,
                const Ordinal ncols_C,
                const Ordinal ncols_Q,
                const Scalar R_bot[],
                const Ordinal ldr_bot,
                const Scalar tau[],
                Scalar C_top[],
                const Ordinal ldc_top,
                Scalar C_bot[],
                const Ordinal ldc_bot,
                Scalar work[]) const
    {
      return default_.apply_pair (applyType, ncols_C, ncols_Q,
                                  R_bot, ldr_bot, tau,
                                  C_top, ldc_top, C_bot, ldc_bot,
                                  work);
    }

  private:
    mutable combine_default_type default_;
  };


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  GER (const magnitude_type alpha,
       const scalar_type x[],
       const Ordinal incx,
       const scalar_type y[],
       const Ordinal incy,
       const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& A) const
  {
    //blas_type ().GER (m, n, alpha, x, incx, y, incy, A, lda);
    constexpr scalar_type ZERO {0.0};
    const Ordinal m = A.dimension_0 ();
    const Ordinal n = A.dimension_1 ();

    Ordinal jy = (incy > 0) ? 1 : 1 - (n-1) * incy;

    if (incx == 1) {
      for (Ordinal j = 0; j < n; ++j) {
        if (y[jy-1] != ZERO) {
          const scalar_type temp = alpha * y[jy-1];
          for (Ordinal i = 0; i < m; ++i) {
            A(i,j) = A(i,j) + x[i] * temp;
          }
        }
        jy += incy;
      }
    }
    else {
      const Ordinal kx = (incx > 0) ? 1 : 1 - (m-1)*incx;
      for (Ordinal j = 0; j < n; ++j) {
        if (y[jy] != ZERO) {
          const scalar_type temp = alpha * y[jy-1];
          Ordinal ix = kx;
          for (Ordinal i = 0; i < m; ++i) {
            A(i,j) = A(i,j) + x[ix-1] * temp;
            ix += incx;
          }
        }
        jy += incy;
      }
    }
  }


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  GEMV (const char trans[],
        const scalar_type alpha,
        const Kokkos::View<const scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& A,
        const scalar_type x[],
        const Ordinal incx,
        const scalar_type beta,
        scalar_type y[],
        const Ordinal incy) const
  {
    using y_vec_type = Kokkos::View<scalar_type*, Kokkos::LayoutLeft, Kokkos::Serial>;
    using x_vec_type = Kokkos::View<const scalar_type*, Kokkos::LayoutLeft, Kokkos::Serial>;
    //blas_type ().GEMV (trans, m, n, alpha, A, lda, x, incx, beta, y, incy);

    const Ordinal m = A.dimension_0 ();
    const Ordinal n = A.dimension_1 ();

    TEUCHOS_TEST_FOR_EXCEPTION
      (incx != 1 || incy != 1, std::logic_error,
       "TSQR::CombineNative::GEMV: Only INCX=1 and INCY=1 cases implemented.");
    const bool no_trans = (trans[0] == 'N' || trans[0] == 'n');
    x_vec_type x_view (x, no_trans ? n : m);
    y_vec_type y_view (y, no_trans ? m : n);

    KokkosBlas::gemv (trans, alpha, A, x_view, beta, y_view);
  }

  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  factor_inner (const Ordinal m,
                const Ordinal n,
                Scalar R[],
                const Ordinal ldr,
                Scalar A[],
                const Ordinal lda,
                Scalar tau[],
                Scalar work[]) const
  {
    const Scalar ZERO(0), ONE(1);
    lapack_type lapack;
    using mat_type = Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>;
    mat_type A_full (A, lda, n);
    mat_type A_view = Kokkos::subview (A_full, std::pair<Ordinal, Ordinal> (0, m), Kokkos::ALL ());

    for (Ordinal k = 0; k < n; ++k) {
      work[k] = ZERO;
    }

    for (Ordinal k = 0; k < n-1; ++k) {
      Scalar& R_kk = R[ k + k * ldr ];
      Scalar* const A_1k = &A[ 0 + k * lda ];
      //Scalar* const A_1kp1 = &A[ 0 + (k+1) * lda ];
      auto A_1kp1 = Kokkos::subview (A_view, std::pair<Ordinal, Ordinal> (0, m),
                                     std::pair<Ordinal, Ordinal> (k+1, n));

      lapack.LARFG (m + 1, &R_kk, A_1k, 1, &tau[k]);
      this->GEMV ("T", ONE, A_1kp1, A_1k, 1, ZERO, work, 1);

      for (Ordinal j = k+1; j < n; ++j) {
        Scalar& R_kj = R[ k + j*ldr ];

        work[j-k-1] += R_kj;
        R_kj -= tau[k] * work[j-k-1];
      }
      this->GER (-tau[k], A_1k, 1, work, 1, A_1kp1);
    }
    Scalar& R_nn = R[ (n-1) + (n-1) * ldr ];
    Scalar* const A_1n = &A[ 0 + (n-1) * lda ];

    lapack.LARFG (m+1, &R_nn, A_1n, 1, &tau[n-1]);
  }


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  apply_inner (const ApplyType& applyType,
               const Ordinal m,
               const Ordinal ncols_C,
               const Ordinal ncols_Q,
               const Scalar A[],
               const Ordinal lda,
               const Scalar tau[],
               Scalar C_top[],
               const Ordinal ldc_top,
               Scalar C_bot[],
               const Ordinal ldc_bot,
               Scalar work[]) const
  {
    const Scalar ZERO(0);
    blas_type blas;

    using Kokkos::ALL;
    using Kokkos::subview;
    using mat_type = Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>;
    using range_type = std::pair<Ordinal, Ordinal>;

    mat_type C_bot_full (C_bot, ldc_bot, ncols_C);
    auto C_bot_view = subview (C_bot_full, range_type (0, m), ALL ());

    //Scalar* const y = work;
    for (Ordinal i = 0; i < ncols_C; ++i)
      work[i] = ZERO;

    Ordinal j_start, j_end, j_step;
    if (applyType == ApplyType::NoTranspose) {
      j_start = ncols_Q - 1;
      j_end = -1; // exclusive
      j_step = -1;
    }
    else {
      j_start = 0;
      j_end = ncols_Q; // exclusive
      j_step = +1;
    }
    for (Ordinal j = j_start; j != j_end; j += j_step) {
      const Scalar* const A_1j = &A[ 0 + j*lda ];

      //blas.GEMV ("T", m, ncols_C, ONE, C_bot, ldc_bot, A_1j, 1, ZERO, &y[0], 1);
      for (Ordinal i = 0; i < ncols_C; ++i) {
        work[i] = ZERO;
        for (Ordinal k = 0; k < m; ++k) {
          work[i] += A_1j[k] * C_bot_view(k, i);
        }

        work[i] += C_top[ j + i*ldc_top ];
      }
      for (Ordinal k = 0; k < ncols_C; ++k) {
        C_top[ j + k*ldc_top ] -= tau[j] * work[k];
      }

      this->GER (-tau[j], A_1j, 1, work, 1, C_bot_view);
    }
  }


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  factor_pair (const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_top,
               const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_bot,
               scalar_type tau[],
               scalar_type work[]) const
  {
    const scalar_type ZERO(0), ONE(1);
    lapack_type lapack;
    blas_type blas;

    const Ordinal n = R_top.dimension_0 ();

    for (Ordinal k = 0; k < n; ++k) {
      work[k] = ZERO;
    }

    for (Ordinal k = 0; k < n-1; ++k) {
      scalar_type& R_top_kk = R_top(k, k);
      scalar_type* const R_bot_1k = &R_bot(0, k);
      //scalar_type* const R_bot_1kp1 = &R_bot(0, k+1);
      auto R_bot_1kp1 = Kokkos::subview (R_bot, std::pair<Ordinal, Ordinal> (0, k+1),
                                         std::pair<Ordinal, Ordinal> (k+1, n));

      // k+2: 1 element in R_top (R_top(k,k)), and k+1 elements in
      // R_bot (R_bot(1:k,k), in 1-based indexing notation).
      lapack.LARFG (k+2, &R_top_kk, R_bot_1k, 1, &tau[k]);
      // One-based indexing, Matlab version of the GEMV call below:
      // work(1:k) := R_bot(1:k,k+1:n)' * R_bot(1:k,k)

      this->GEMV ("T", ONE, R_bot_1kp1, R_bot_1k, 1, ZERO, work, 1);

      for (Ordinal j = k+1; j < n; ++j) {
        scalar_type& R_top_kj = R_top(k, j);
        work[j-k-1] += R_top_kj;
        R_top_kj -= tau[k] * work[j-k-1];
      }
      this->GER (-tau[k], R_bot_1k, 1, work, 1, R_bot_1kp1);
    }
    scalar_type& R_top_nn = R_top(n-1, n-1);
    scalar_type* const R_bot_1n = &R_bot(0, n-1);

    // n+1: 1 element in R_top (n,n), and n elements in R_bot (the
    // whole last column).
    lapack.LARFG (n+1, &R_top_nn, R_bot_1n, 1, &tau[n-1]);
  }


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  factor_pair (const Ordinal n,
               Scalar R_top[],
               const Ordinal ldr_top,
               Scalar R_bot[],
               const Ordinal ldr_bot,
               Scalar tau[],
               Scalar work[]) const
  {
    Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial> R_top_full (R_top, ldr_top, n);
    Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial> R_bot_full (R_bot, ldr_bot, n);
    if (ldr_top == n) {
      if (ldr_bot == n) {
        this->factor_pair (R_top_full, R_bot_full, tau, work);
      }
      else {
        auto R_bot_view = Kokkos::subview (R_bot_full, std::pair<Ordinal, Ordinal> (0, n), Kokkos::ALL ());
        this->factor_pair (R_top_full, R_bot_view, tau, work);
      }
    }
    else {
      auto R_top_view = Kokkos::subview (R_top_full, std::pair<Ordinal, Ordinal> (0, n), Kokkos::ALL ());
      if (ldr_bot == n) {
        this->factor_pair (R_top_view, R_bot_full, tau, work);
      }
      else {
        auto R_bot_view = Kokkos::subview (R_bot_full, std::pair<Ordinal, Ordinal> (0, n), Kokkos::ALL ());
        this->factor_pair (R_top_view, R_bot_view, tau, work);
      }
    }
  }


  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  apply_pair (const ApplyType& applyType,
              const Ordinal ncols_C,
              const Ordinal ncols_Q,
              const scalar_type R_bot[],
              const Ordinal ldr_bot,
              const scalar_type tau[],
              scalar_type C_top[],
              const Ordinal ldc_top,
              scalar_type C_bot[],
              const Ordinal ldc_bot,
              scalar_type work[]) const
  {
    using const_mat_type = Kokkos::View<const scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>;
    using nonconst_mat_type = Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>;

    const_mat_type R_bot_full (R_bot, ldr_bot, ncols_Q);
    nonconst_mat_type C_top_full (C_top, ldc_top, ncols_C);
    nonconst_mat_type C_bot_full (C_bot, ldc_bot, ncols_C);

    auto R_bot_view = Kokkos::subview (R_bot_full,
                                       std::pair<Ordinal, Ordinal> (0, ncols_Q),
                                       Kokkos::ALL ());
    auto C_top_view = Kokkos::subview (C_top_full,
                                       std::pair<Ordinal, Ordinal> (0, ncols_C),
                                       Kokkos::ALL ());
    auto C_bot_view = Kokkos::subview (C_bot_full,
                                       std::pair<Ordinal, Ordinal> (0, ncols_C),
                                       Kokkos::ALL ());
    this->apply_pair (applyType, R_bot_view, tau, C_top_view, C_bot_view, work);
  }

  template< class Ordinal, class Scalar >
  void
  CombineNative< Ordinal, Scalar, false >::
  apply_pair (const ApplyType& applyType,
              const Kokkos::View<const scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& R_bot, // ncols_Q
              const scalar_type tau[],
              const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& C_top, // ncols_C
              const Kokkos::View<scalar_type**, Kokkos::LayoutLeft, Kokkos::Serial>& C_bot,
              scalar_type work[]) const
  {
    constexpr scalar_type ZERO {0.0};

    const Ordinal ncols_C = C_top.dimension_1 ();
    const Ordinal ncols_Q = R_bot.dimension_1 ();

    for (Ordinal i = 0; i < ncols_C; ++i) {
      work[i] = ZERO;
    }

    Ordinal j_start, j_end, j_step;
    if (applyType == ApplyType::NoTranspose) {
      j_start = ncols_Q - 1;
      j_end = -1; // exclusive
      j_step = -1;
    }
    else {
      j_start = 0;
      j_end = ncols_Q; // exclusive
      j_step = +1;
    }
    for (Ordinal j_Q = j_start; j_Q != j_end; j_Q += j_step) {
      // Using Householder reflector stored in column j_Q of R_bot
      const scalar_type* const R_bot_col = &R_bot(0, j_Q);

      // In 1-based indexing notation, with k in 1, 2, ..., ncols_C
      // (inclusive): (Output is length ncols_C row vector)
      //
      // work(1:j) := R_bot(1:j,j)' * C_bot(1:j, 1:ncols_C) - C_top(j, 1:ncols_C)
      for (Ordinal j_C = 0; j_C < ncols_C; ++j_C) {
        // For each column j_C of [C_top; C_bot], update row j_Q
        // of C_top and rows 1:j_Q of C_bot.  (Again, this is in
        // 1-based indexing notation.

        scalar_type work_j_C = ZERO;
        const scalar_type* const C_bot_col = &C_bot(0, j_C);

        for (Ordinal k = 0; k <= j_Q; ++k)
          work_j_C += R_bot_col[k] * C_bot_col[k];

        work_j_C += C_top(j_Q, j_C);
        work[j_C] = work_j_C;
      }
      for (Ordinal j_C = 0; j_C < ncols_C; ++j_C) {
        C_top(j_Q, j_C) -= tau[j_Q] * work[j_C];
      }
      this->GER (-tau[j_Q], R_bot_col, 1, work, 1, C_bot);
    }
  }
} // namespace TSQR



#endif // __TSQR_CombineNative_hpp
