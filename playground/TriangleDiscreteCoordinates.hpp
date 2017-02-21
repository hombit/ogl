//
// Created by Konstantin Malanchev on 21/02/2017.
//

#include <boost/iterator/filter_iterator.hpp>
#include <cmath>
#include <functional>
#include <iostream>
#include <vector>


#ifndef TUTORIALS_TRIANGLEDESCRETECOORDINATES_HPP
#define TUTORIALS_TRIANGLEDESCRETECOORDINATES_HPP


template<typename T>
T exp2_int(T x){
    T y(1);
    for ( T i = 0; i < x; ++i ){
        y *= 2;
    }
    return y;
}


template <typename T_INDEX>
class TriangleDiscreteCoordinates {
public:
    struct DiscreteCoordinate{
        const size_t rho;
        const size_t psi;
        DiscreteCoordinate(size_t rho, size_t psi): rho(rho), psi(psi){};
    };
    typedef typename std::vector<DiscreteCoordinate> dcvec;
    typedef typename dcvec::const_iterator dcvec_const_it;

protected:
    dcvec idx;

public:
    const size_t tr;
    const size_t splits;
    const size_t rho_size;
    const size_t size;

    TriangleDiscreteCoordinates(size_t triangles, size_t bin_splits):
            tr( triangles ),
            splits( bin_splits ),
            rho_size( exp2_int(splits) + 1 ),
            size( tr * rho_size * (rho_size - 1) / 2 + 1 ){
        for ( size_t rho = 0; rho < rho_size; ++rho ){
            for ( size_t psi = 0; psi < psi_size(rho); ++psi ){
                idx.emplace_back(rho, psi);
            }
        }
    }

    dcvec_const_it begin() const{
        return idx.cbegin();
    }

    dcvec_const_it end() const{
        return idx.cend();
    }

    boost::filter_iterator<std::function<bool(DiscreteCoordinate)>, dcvec_const_it> triangle_begin(size_t triangle) const{
        std::function<bool(DiscreteCoordinate)> predicator = [this, triangle](DiscreteCoordinate x) {
            return this->what_triangle(x) == triangle;
        };
        return boost::make_filter_iterator(
                predicator,
                begin(), end()
        );
    }

    boost::filter_iterator<std::function<bool(DiscreteCoordinate)>, dcvec_const_it> triangle_end(size_t triangle) const{
        std::function<bool(DiscreteCoordinate)> predicator = [this, triangle](DiscreteCoordinate x) {
            return this->what_triangle(x) == triangle;
        };
        return boost::make_filter_iterator(
                predicator,
                end(), end()
        );
    }

    DiscreteCoordinate coordinate(size_t index) const{
        return idx[index];
    }

    size_t what_triangle(size_t rho, size_t psi) const{
        psi %= psi_size(rho);
        return psi / psi_triangle_size(rho);
    }
    size_t what_triangle(DiscreteCoordinate dc) const{
        return what_triangle(dc.rho, dc.psi);
    }

    size_t psi_size(size_t rho) const{
        if ( rho == 0 ){
            return 1;
        }
        return tr * rho;
    }

    size_t psi_triangle_size(size_t rho) const{
        if ( rho == 0 ){
            return 1;
        }
        return rho;
    }

    T_INDEX index(size_t rho, size_t psi) const{
        if ( rho == 0 ){
            return 0;
        }
        psi %= psi_size(rho);
        return static_cast<T_INDEX>( tr * rho * (rho - 1) / 2 + psi + 1 );
    }
    T_INDEX index(DiscreteCoordinate dc) const{
        return index(dc.rho, dc.psi);
    }

    std::vector<T_INDEX> get_triangles() const{
        std::vector<T_INDEX> triangles;
        for ( size_t rho = 1; rho < rho_size; ++rho ){
            size_t delta_psi = 0;
            for ( size_t psi = 0; psi < psi_size(rho); ++psi ){
                if ( psi != 0  and  psi % psi_triangle_size(rho) == 0 ){
                    delta_psi++;
                }

                    triangles.push_back(index(rho,     psi));
                    triangles.push_back(index(rho,     psi + 1));
                    triangles.push_back(index(rho - 1, psi - delta_psi));

                if ( rho < rho_size - 1 ){
                    triangles.push_back(index(rho,     psi));
                    triangles.push_back(index(rho,     psi + 1));
                    triangles.push_back(index(rho+1,   psi + delta_psi + 1));
                }
            }
        }
        return triangles;
    }
};


#endif //TUTORIALS_TRIANGLEDESCRETECOORDINATES_HPP
