//
// Created by Konstantin Malanchev on 21/02/2017.
//

#ifndef TUTORIALS_DESCRITETOGEOMETRIC_HPP
#define TUTORIALS_DESCRITETOGEOMETRIC_HPP

#include <cmath>
#include <vector>
#include <glm/gtx/polar_coordinates.hpp>

#include "TriangleDiscreteCoordinates.hpp"


template<typename T>
std::vector<glm::vec3> northern_hemisphere(const TriangleDiscreteCoordinates<T> &tdc){
    typedef glm::vec3::value_type value_type;

    std::vector<glm::vec3> vertices(tdc.size);

    const glm::vec3 A = glm::euclidean(glm::vec2(M_PI_2, 0));
    for ( size_t t = 0; t < tdc.tr; ++t ){
        const glm::vec3 B = glm::euclidean(glm::vec2(0, M_PI_2 * t));
        const glm::vec3 C = glm::euclidean(glm::vec2(0, M_PI_2 * (t + 1)));
        for ( auto it = tdc.triangle_begin(t); it != tdc.triangle_end(t); ++it ){
            const value_type rho = it->rho;
            const auto psi_length = tdc.psi_triangle_size(it->rho);
            const value_type psi = it->psi % psi_length;
            value_type Aw = (value_type) (tdc.rho_size - 1 - rho) / (tdc.rho_size - 1);
//            Aw = (value_type) 1 - cos( M_PI_2 * Aw );
            value_type Bw = (value_type) (psi_length - 1 - psi) / (psi_length - 1);
            value_type Cw = (value_type) 1 - Bw;
            if ( it->rho == 0 ) {
                Bw = 0;
                Cw = 0;
            }
            if ( it->rho == 1 ){
                Bw = 1;
                Cw = 0;
            }
            std::cout << Bw << std::endl;
            vertices[tdc.index(*it)] = glm::normalize(
                    Aw * A + Bw * B + Cw * C
            );
        }
    }

    return vertices;
}


template<typename T>
void sphere(size_t splits,
            std::vector<glm::vec3> &vertices,
            std::vector<T>         &indeces,
            std::vector<glm::vec2> &uvs,
            std::vector<glm::vec3> &normals){
    auto tdc = TriangleDiscreteCoordinates<T>(4, splits);

    auto v = northern_hemisphere(tdc);
    vertices.insert(vertices.begin(), v.begin(), v.end());

    auto ind = tdc.get_triangles();
    indeces.insert(indeces.begin(), ind.begin(), ind.end());

    uvs.clear();
    for ( size_t i = 0; i < tdc.size; ++i ){
        const auto polar3 = glm::polar(vertices[i]);
        uvs.emplace_back( polar3.x, polar3.y );
    }

    normals.insert(normals.begin(), vertices.begin(), vertices.end());
}


#endif //TUTORIALS_DESCRITETOGEOMETRIC_HPP
