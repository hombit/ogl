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
            const auto rho        = static_cast<value_type>( it->rho );
            const auto rho_length = static_cast<value_type>( tdc.rho_size );
            const auto psi_length = static_cast<value_type>( tdc.psi_triangle_size(it->rho) );
            const auto psi        = static_cast<value_type>( it->psi % tdc.psi_triangle_size(it->rho) );

            value_type alpha ( 1 - rho / (rho_length - 1) );
            value_type beta  ( (1 - alpha) * ( 1 - psi / (psi_length - 1) ) );
            value_type gamma ( 1 - alpha - beta );
            if ( it->rho == 0 ) {
                alpha = 1;
                beta  = 0;
                gamma = 0;
            }
            if ( it->rho == 1 ){
                beta  = 1 - alpha;
                gamma = 0;
            }
            vertices[tdc.index(*it)] = glm::normalize( alpha * A + beta * B + gamma * C );
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
    typedef glm::vec3::value_type value_type;

    const auto tdc = TriangleDiscreteCoordinates<T>(4, splits);

    auto verts = northern_hemisphere(tdc);
    // Add southern hemisphere vertices
    const size_t last_line = tdc.tr * (tdc.rho_size - 2) * (tdc.rho_size - 3) / 2 + 1;
    for ( size_t i = 0; i < last_line; ++i ){
        auto southern_vert = verts[i];
        southern_vert.y *= -1;
        verts.push_back(southern_vert);
    }

    vertices.insert(vertices.begin(), verts.begin(), verts.end());

    auto ind = tdc.get_triangles();
    // generate southern hemisphere triangles
    auto southern_ind = ind;
    for (auto &i : southern_ind){
        if (i < last_line){
            i += tdc.size;
        }
    }
    ind.insert(ind.end(), southern_ind.begin(), southern_ind.end());

    indeces.insert(indeces.begin(), ind.begin(), ind.end());

    uvs.clear();
    for ( auto &vert : verts ){
        const auto xz_dist = static_cast<value_type>( vert.x * vert.x + vert.z * vert.z );
        const auto theta   = static_cast<value_type>( atan(xz_dist / vert.y) );
        const auto phi     = static_cast<value_type>( atan2( vert.x, vert.z ) );
        const auto v       = static_cast<value_type>( (theta + M_PI_2) / M_PI );
        const auto u       = static_cast<value_type>( (phi + M_PI) / ( 2 * M_PI ) );
        uvs.emplace_back( u, v );
    }

    normals.insert(normals.begin(), vertices.begin(), vertices.end());
}


#endif //TUTORIALS_DESCRITETOGEOMETRIC_HPP
