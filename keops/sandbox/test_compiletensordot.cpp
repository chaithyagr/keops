#include <iostream>
#include <tuple>
#include <array>

constexpr std::size_t operator "" _z(unsigned long long n) {
  return n;
}

#define Ind(...) std::index_sequence<__VA_ARGS__>

// TensorDot(A,B,Ind(2,2,2), Ind(2,2), Ind(0,1), Ind(0,1))
// TensorDot(A,B,{2,2,2}, {2,2}, {0,1}, {0,1))

using DimFa = Ind(2, 2, 2);
using DimFb = Ind(2, 2);
using ContFa = Ind(2);
using ContFb = Ind(0);

using ContFa4 = Ind(1,2);
using ContFb4 = Ind(0,1);


using DimFa6 = Ind(5, 4, 3);
using DimFb6 = Ind(4, 3, 2);
using ContFa6 = Ind(1,2);
using ContFb6 = Ind(0,1);

using ContFa8 = Ind(1,2,3);

template<size_t... DIMFA, size_t... DIMFB, size_t... CONTFA, size_t... CONTFB>
static constexpr std::tuple<std::array<size_t, sizeof...(DIMFA)>,
                            std::array<size_t, sizeof...(DIMFB)>,
                            std::array<size_t, sizeof...(DIMFA) - sizeof...(CONTFA)>,
                            std::array<size_t, sizeof...(DIMFB) - sizeof...(CONTFB)>,
                            std::array<size_t,
                                       sizeof...(DIMFA) + sizeof...(DIMFB) - sizeof...(CONTFA) - sizeof...(CONTFB)>,
                            std::array<size_t, sizeof...(CONTFB)>,
                            std::array<size_t, sizeof...(DIMFA) + sizeof...(DIMFB) - sizeof...(CONTFA)>>
build_array4(std::index_sequence<DIMFA...>,
             std::index_sequence<DIMFB...>,
             std::index_sequence<CONTFA...>,
             std::index_sequence<CONTFB...>) noexcept {

  // Cast index_sequence to array
  auto indices_a = std::array<size_t, sizeof...(DIMFA)>{DIMFA...};
  auto indices_b = std::array<size_t, sizeof...(DIMFB)>{DIMFB...};
  auto indices_contdim_a = std::array<size_t, sizeof...(CONTFA)>{CONTFA...};
  auto indices_contdim_b = std::array<size_t, sizeof...(CONTFB)>{CONTFB...};

  // get size of the contraction
  constexpr size_t size_keep_dim_a = indices_a.size() - indices_contdim_a.size();
  constexpr size_t size_keepdim_b = indices_b.size() - indices_contdim_b.size();




  // dim_keep : contains the list of kept dim
  std::array<size_t, size_keepdim_b + size_keep_dim_a> dim_keep{};
  std::array<size_t, size_keep_dim_a> dim_keep_a{};
  std::array<size_t, size_keepdim_b> dim_keep_b{};
  for (size_t i = 0; i < size_keep_dim_a; i++) {
    dim_keep_a[i] = indices_a[i];
    dim_keep[i] = indices_a[i];
  }

  for (size_t i = 0; i < size_keepdim_b; i++) {
    dim_keep_b[i] = indices_b[indices_contdim_b.size() + i];
    dim_keep[i + size_keep_dim_a] = dim_keep_b[i];
  }

  // contdim
  std::array<size_t, indices_contdim_a.size()> dim_cont{};
  for (size_t i = 0; i < indices_contdim_a.size(); i++) {
    dim_cont[i] = indices_a[size_keep_dim_a + i];
  }

  // dim_tot : contains all indices in that order [dim_keep_a, dim_keep_b, dim_cont]
  std::array<size_t, indices_contdim_a.size() + dim_keep.size()> dim_tot{};
  for (size_t i = 0; i < dim_keep.size(); i++) {
    dim_tot[i] = dim_keep[i];
  }
  for (size_t i = 0; i < indices_contdim_a.size(); i++) {
    dim_tot[dim_keep.size() + i] = dim_cont[i];
  }

  return std::make_tuple(indices_a, indices_b,
                         dim_keep_a, dim_keep_b, dim_keep,
                         dim_cont,
                         dim_tot);
}

template<size_t shape_index, size_t shape_size>
struct Looper {
  template<typename Functor>
  void operator()(const std::array<size_t, shape_size> &shape, Functor functor) {
    for (size_t index = 0; index < shape[shape_index]; ++index) {
      Looper<shape_index + 1, shape_size>()(
          shape,
          [index, &functor](auto... tail) { functor(std::tuple_cat(std::tuple<size_t>(index), tail...)); });
    }
  }
};

template<size_t shape_size>
struct Looper<shape_size, shape_size> {
  template<typename Functor>
  void operator()(const std::array<size_t, shape_size> &, Functor functor) {
    functor();
  }
};

template<size_t shape_size, typename Functor>
void loop(const std::array<size_t, shape_size> &shape, Functor functor) {
  Looper<0, shape_size>()(shape, functor);
}

//constexpr unsigned cilog2(unsigned val) { return val ? 1 + cilog2(val >> 1) : -1; }

template<size_t size1, size_t size2>
constexpr std::tuple<size_t, size_t, size_t> kd(std::array<size_t, size1> dim_a,
                                                std::array<size_t, size2> dim_b,
                                                size_t i,
                                                size_t j,
                                                size_t k) {
  size_t kda = dim_a[1] * dim_a[2] * i + dim_a[2] * j;

  size_t kdb = k;
  size_t I = kda + kdb;
  return std::make_tuple(I, kda, kdb);
}

template<size_t size_list_dim_a, size_t size_list_dim_b, size_t size_list_contdim>
constexpr std::tuple<size_t, size_t, size_t> kdvar(std::array<size_t, size_list_dim_a> list_dim_a,
                                                   std::array<size_t, size_list_dim_b> list_dim_b,
                                                   std::array<size_t, size_list_dim_a + size_list_dim_b - size_list_contdim> list_indices_tot)          // {i,j,k}
{

  constexpr size_t size_keep_dim_a = size_list_dim_a - size_list_contdim;
  constexpr size_t size_keep_dim_b = size_list_dim_b - size_list_contdim;

  std::array<size_t, size_list_contdim> list_indices_contdim;
  for (size_t i = 0; i < size_list_contdim; i++) {
    list_indices_contdim[i] = list_indices_tot[(size_keep_dim_a + size_keep_dim_b) + i];   // {3}
  }

  // FA --------------------------
  std::array<size_t, size_keep_dim_a> list_indices_keepdim_a;
  for (size_t i = 0; i < (size_keep_dim_a); i++) {
    list_indices_keepdim_a[i] = list_indices_tot[i];       // {0,1}
  }

  std::array<size_t, size_list_dim_a> list_stride_dim_a;
  for (size_t i = 0; i < size_list_dim_a; i++) {
    list_stride_dim_a[i] = 1;
    for (size_t j=i+1; j < size_list_dim_a; j++)
      list_stride_dim_a[i] *= list_dim_a[j];
  }

  size_t kda = 0;
  for (size_t i = 0; i < (size_keep_dim_a); i++) {
    kda += list_stride_dim_a[i] * list_indices_keepdim_a[i];
  }

  for (size_t i = 0; i < (size_list_contdim); i++) {
    kda += list_stride_dim_a[size_keep_dim_a + i] * list_indices_contdim[i];
  }


  // FB --------------------------
  std::array<size_t, size_keep_dim_b> list_indices_keepdim_b;
  for (size_t i = 0; i < (size_keep_dim_b); i++) {
    list_indices_keepdim_b[i] = list_indices_tot[size_keep_dim_a + i];               // {2}
  }

  std::array<size_t, size_list_dim_b> list_stride_dim_b;
  for (size_t i = 0; i < size_list_dim_b; i++) {
    list_stride_dim_b[i] = 1;
    for (size_t j=i+1; j < size_list_dim_b; j++ )
      list_stride_dim_b[i] *= list_dim_b[j];                                                       // {0,1}
  }

  size_t kdb = 0;
  for (size_t i = 0; i < (size_keep_dim_b); i++) {
    kdb += list_stride_dim_b[size_list_dim_b-1 - i] * list_indices_keepdim_b[i];
  }
  for (size_t i = 0; i < (size_list_contdim); i++) {
    kdb += list_stride_dim_b[i] * list_indices_contdim[i];
  }


  // ------------------

  size_t I = 0;


  std::array<size_t, size_keep_dim_a + size_keep_dim_b> list_stride_keepdim;
  for (size_t i = 0; i < size_keep_dim_a  + size_keep_dim_b; i++) {
    list_stride_keepdim[i] = 1;
    for (size_t j=i+1; j < size_keep_dim_a  + size_keep_dim_b; j++ )
      list_stride_keepdim[i] *= ( j < size_keep_dim_a) ?  list_dim_a[j] : list_dim_b[j-size_keep_dim_a+size_list_contdim];                                                       // {0,1}
  }

  for (size_t i = 0; i < size_keep_dim_a + size_keep_dim_b  ; i++) {
    //I += ((i == 0) ? 1 : list_dim_b[i]) * (list_indices_keepdim_b[i] + I);
    I += list_stride_keepdim[i] * (( i < size_keep_dim_a) ? list_indices_keepdim_a[i] : list_indices_keepdim_b[i - size_keep_dim_a]);
  }

  std::cout << "(" << list_indices_tot[0] << "," << list_indices_tot[1] <<  "," <<list_indices_tot[2] <<  "," <<list_indices_tot[3] <<")     " << kda << " " << kdb << " " << I << std::endl;

  return std::make_tuple(I, kda, kdb);
}

template<size_t NumberOfKeepDim>
constexpr size_t dimout(std::array<size_t, NumberOfKeepDim> keep_dim) {
  size_t out = 1;
  for (size_t i : keep_dim)
    out *= i;

  return out;
}

//template< size_t ... A>
//static constexpr std::array< size_t, sizeof...(A)> toArray( std::index_sequence<A... >) noexcept {
//return std::array<size_t, sizeof...(A ) > { A... };
//}
//

template<typename tuple_t>
constexpr auto get_array_from_tuple(tuple_t &&tuple) {
  constexpr auto get_array = [](auto &&... x) { return std::array{std::forward<decltype(x)>(x) ...}; };
  return std::apply(get_array, std::forward<tuple_t>(tuple));
}

template<size_t, class T>
using T_ = T;

template<class T, size_t... Is>
auto gen(std::index_sequence<Is...>) { return std::tuple<T_<Is, T>...>{}; }

template<class T, size_t N>
auto gen() { return gen<T>(std::make_index_sequence<N>{}); }

struct KD_T {

};

// constexpr std::tuple<size_t,size_t,size_t> kd(std::array<size_t, size1> dim_a, std::array<size_t, size2> dim_b, size_t i, size_t j , size_t k)

int main() {

  double FA[8] = {4.4, 5.4, 6.2, 6.5, 7.5, 6.1, 8.7, 1.3};
  double FB[4] = {1.4, 1.2, 1.5, 1.22};

  // generate tuple (at compile time)
  constexpr auto ma4 = build_array4(DimFa(),
                                    DimFb(),
                                    ContFa(),
                                    ContFb());

  // constexpr size_t sum_dim_a = 2;
  // constexpr size_t keep_dim_a[2] = [0,1];

  // constexpr size_t sum_dim_b[1] = 0;
  // constexpr size_t keep_dim_b[1] = [1];

  // print (at runtime)

  constexpr size_t dimout_var = dimout(std::get<4>(ma4));
  double out[dimout_var];
  std::fill(out, out + dimout_var, 0);

  const auto &my_lambda = [&out, &FA, &FB, &ma4](decltype(gen<size_t, std::get<6>(ma4).size()>()) it) {
    const auto &dim_a = std::get<0>(ma4);
    const auto &dim_b = std::get<1>(ma4);

    std::tuple KD = kd(dim_a, dim_b, std::get<0>(it), std::get<1>(it), std::get<2>(it));
    size_t I = std::get<0>(KD);
    size_t kda = std::get<1>(KD);
    size_t kdb = std::get<2>(KD);

    out[I] += FA[kda + std::get<3>(it)] * FB[kdb + dim_b[1] * std::get<3>(it)];
  };

  loop(std::get<6>(ma4), my_lambda);

  for (auto i = 0; i < 8; i++) {
    std::cout << "out  = " << out[i] << std::endl;
  }

  // -------------------------------------------------------------------------------------------------------------------------------------

  double out3[dimout_var];
  std::fill(out3, out3 + dimout_var, 0);

  constexpr const size_t indices_number = std::get<6>(ma4).size();

  const auto &my_lambda2 = [&out3, &FA, &FB, &ma4](decltype(gen<size_t, indices_number>()) it) {
    const auto &dim_a = std::get<0>(ma4);
    const auto &dim_b = std::get<1>(ma4);

    std::tuple KD = kdvar<3, 2, 1>(dim_a, dim_b, get_array_from_tuple(it));

    size_t I = std::get<0>(KD);
    size_t kda = std::get<1>(KD);
    size_t kdb = std::get<2>(KD);

    out3[I] += FA[kda] * FB[kdb];
  };

  loop(std::get<6>(ma4), my_lambda2);

  for (auto i = 0; i < 8; i++) {
    std::cout << "out3 = " << out3[i] << std::endl;
  }

  // -------------------------------------------------------------------------------------------------------------------------------------
  double out2[2 * 2 * 2] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (size_t i = 0; i < 2; i++)
    for (size_t j = 0; j < 2; j++)
      for (size_t k = 0; k < 2; k++)
        for (size_t l = 0; l < 2; l++) {
          size_t kda = 4 * i + 2 * j + l;
          size_t kdb = l * 2 + k;
          size_t I = 4 * i + 2 * j + k;
          std::cout << "(" << i << "," << j <<  "," << k <<  "," << l <<")     " << kda << " " << kdb << " " << I << std::endl;
          out2[4 * i + 2 * j + k] += FA[4 * i + 2 * j + l] * FB[l * 2 + k];
        }

  for (auto i = 0; i < 8; i++) {
    std::cout << "out2 = " << out2[i] << std::endl;
  }

  std::cout << std::endl;


  // -------------------------------------------------------------------------------------------------------------------------------------
  double out4[2] = {0, 0};

  for (size_t i = 0; i < 2; i++)
      for (size_t k = 0; k < 2; k++)
        for (size_t l = 0; l < 2; l++) {

          out4[ i ] += FA[4 * i + 2 * k + l] * FB[k * 2 + l];
        }

  for (auto i = 0; i < 2; i++) {
    std::cout << "out4 = " << out4[i] << std::endl;
  }

  double out5[2] = {0, 0};
  constexpr auto ma5 = build_array4(DimFa(),
                                    DimFb(),
                                    ContFa4(),
                                    ContFb4());

  constexpr const size_t indices_number4 = std::get<6>(ma5).size();
  const auto &my_lambda4 = [&out5, &FA, &FB, &ma5](decltype(gen<size_t, indices_number4>()) it) {
    const auto &dim_a = std::get<0>(ma5);
    const auto &dim_b = std::get<1>(ma5);

    std::tuple KD = kdvar<3, 2, 2>(dim_a,
                                   dim_b,
                                   get_array_from_tuple(it));
    size_t I = std::get<0>(KD);
    size_t kda = std::get<1>(KD);
    size_t kdb = std::get<2>(KD);

    out5[I] += FA[kda] * FB[kdb];
  };

  loop(std::get<6>(ma5), my_lambda4);

  for (auto i = 0; i < 2; i++) {
    std::cout << "out5 = " << out5[i] << std::endl;
  }


  // ----------------------------------------
  double FAA[60] =  {7, 9, 9, 5, 8, 3, 6, 9, 6, 0, 5, 7, 3, 4, 3, 5, 3, 3,0, 9, 9,6, 0, 3,3, 7, 0,8, 6, 0,6, 1, 3,1, 4, 7,3, 9, 8,8, 3, 7,2, 3, 1,9, 5, 7,7, 5, 9,7, 0, 1,9, 7, 5,0, 3, 8};
  double FBB[24] =  {6, 4,2, 9,9, 5,1, 6,7, 8,2, 4,1, 9,7, 8,5, 4,3, 2,3, 8,5, 7};

  double out7[10] =  {0, 0, 0, 0 ,0, 0, 0, 0, 0 ,0};

  for (size_t i = 0; i < 5; i++)
    for (size_t j = 0; j < 2; j++)
      for (size_t k = 0; k < 4; k++)
        for (size_t l = 0; l < 3; l++) {

          size_t kda = 12 * i + 3 * k + l;
          size_t kdb = 6 * k + 2 * l + j;
          size_t I = 2 * i + j;
          std::cout << "(" << i << "," << j <<  "," << k <<  "," << l <<")     " << kda << " " << kdb << " " << I << std::endl;
          out7[2 * i + j] += FAA[ 12 * i + 3 * k + l] * FBB[6 * k + 2 * l + j];
      }

  for (auto i = 0; i < 10; i++) {
    std::cout << "out7 = " << out7[i] << std::endl;
  }



  double out6[10] = {0, 0, 0, 0 ,0, 0, 0, 0, 0 ,0};
  constexpr auto ma6 = build_array4(DimFa6(),
                                    DimFb6(),
                                    ContFa6(),
                                    ContFb6());

  constexpr const size_t indices_number6 = std::get<6>(ma6).size();
  const auto &my_lambda6 = [&out6, &FAA, &FBB, &ma6](decltype(gen<size_t, indices_number6>()) it) {
  const auto &dim_a6 = std::get<0>(ma6);
  const auto &dim_b6 = std::get<1>(ma6);

    std::tuple KD = kdvar<3, 3, 2>(dim_a6,
                                   dim_b6,
                                   get_array_from_tuple(it));
    size_t I = std::get<0>(KD);
    size_t kda = std::get<1>(KD);
    size_t kdb = std::get<2>(KD);

    out6[I] += FAA[kda] * FBB[kdb];
  };

  loop(std::get<6>(ma6), my_lambda6);

  for (auto i = 0; i < 10; i++) {
    std::cout << "out6 = " << out6[i] << std::endl;
  }

  // -----------------

  double out8[1] = {0};
  constexpr auto ma8 = build_array4(DimFa6(),
                                    DimFa6(),
                                    ContFa8(),
                                    ContFa8());

  constexpr const size_t indices_number8 = std::get<6>(ma8).size();
  const auto &my_lambda8 = [&out8, &FAA, &FBB, &ma8](decltype(gen<size_t, indices_number8>()) it) {
    const auto &dim_a8 = std::get<0>(ma8);
    const auto &dim_b8 = std::get<1>(ma8);

    std::tuple KD = kdvar<3, 3, 3>(dim_a8,
                                   dim_b8,
                                   get_array_from_tuple(it));
    size_t I = std::get<0>(KD);
    size_t kda = std::get<1>(KD);
    size_t kdb = std::get<2>(KD);

    out8[I] += FAA[kda] * FAA[kdb];
  };

  loop(std::get<6>(ma8), my_lambda8);

  for (auto i = 0; i < 1; i++) {
    std::cout << "out8 = " << out8[i] << std::endl;
  }
}


/*

 import numpy as np
 a = np.array([4.4, 5.4, 6.2, 6.5, 7.5, 6.1, 8.7, 1.3]).reshape(2,2,2)
 b = np.array([1.4, 1.2, 1.5, 1.22]).reshape(2,2)
 print(np.tensordot(a,b,axes=([2],[0])))

 print(np.tensordot(a,b,axes=([1,2],[0,1]))) # out4


 a= np.array([[[7, 9, 9], [5, 8, 3], [6, 9, 6], [0, 5, 7]], [[3, 4, 3], [5, 3, 3],[0, 9, 9],[6, 0, 3]],[[3, 7, 0],[8, 6, 0],[6, 1, 3],[1, 4, 7]],
[[3, 9, 8],[8, 3, 7],[2, 3, 1],[9, 5, 7]],[[7, 5, 9],[7, 0, 1],[9, 7, 5],[0, 3, 8]]])

b = np.array([[[6, 4],[2, 9],[9, 5]],[[1, 6],[7, 8],[2, 4]],[[1, 9],[7, 8],[5, 4]],[[3, 2],[3, 8],[5, 7]]])

np.tensordot(a,b,axes=([1,2],[0,1]))
array([[357, 499],
       [226, 270],
       [160, 328],
       [256, 386],
       [274, 401]])

 */