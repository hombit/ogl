#include <stdio.h>
#include <iostream>
#include <array>
#include <algorithm>
#include <vector>

#include <GL/glew.h>

#include <glfw3.h>
GLFWwindow* window;

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/polar_coordinates.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/texture.hpp>

#include "TriangleDiscreteCoordinates.hpp"
#include "DescriteToGeometric.hpp"




double ScrollXOffset = 0.;
double ScrollYOffset = 0.;
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    ScrollXOffset += xoffset;
    ScrollYOffset += yoffset;
}


class MVP{
protected:
	const float scale0;
	const float translate_x;
	float scale;
    const float mouseSpeed = 0.005f;
    float lat = 0.0f;
    float lon = 0.0f;

    void positionFromInput(){
        // Get coursor position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Get window size
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        lon += mouseSpeed * float(width/2  - xpos);
        lat += mouseSpeed * float(height/2 - ypos);

		// Set scale via scroll
        scale = (float) exp(ScrollYOffset) * scale0;
    }

public:
    MVP(float scale, float translate_x=0): scale0(scale), scale(scale), translate_x(translate_x){
        glfwSetScrollCallback(window, ScrollCallback);
		positionFromInput();
    };

	mat4 getProjectionMatrix(){
		return ortho(
				-scale,scale,
				-scale,scale,
				0.0f,100.0f
		);
	}

	mat4 getViewMatrix(vec2 spherical_coordinates){
		return lookAt(
				scale * euclidean(spherical_coordinates),
				vec3(0,0,0), // and looks at the origin
				vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
		);
	}

	mat4 getViewMatrix(){
		return getViewMatrix(vec2(lat, lon));
	}

	mat4 getTranslationMatrix(){
		return translate(vec3(translate_x,0,0));
	}

	mat4 getRotationMatrix(){
		return mat4(1);
	}

	mat4 getScaleMatrix(){
		return mat4(1);
	}

	mat4 getModelMatrix(){
		return getTranslationMatrix() * getRotationMatrix() * getScaleMatrix();
	}

	mat4 mv(){
		return getViewMatrix() * getModelMatrix();
	}

	mat4 mvp(){
		return getProjectionMatrix() * getViewMatrix() * getModelMatrix();
	}
};


int playground()
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	const int width = 512;
	const int height = 512;
	window = glfwCreateWindow( width, height, "Playground", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
    glewExperimental = (GLboolean) true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
//	glEnable(GL_CULL_FACE);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Blue background
	glClearColor(0.2f, 0.3f, 0.4f, 0.0f);

    // Create Vertex Array Object (VAO)
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint programID = LoadShaders(
            "playground/VertexShader.glsl",
            "playground/FragmentShader.glsl"
    );

	GLint MVP_ID =   glGetUniformLocation(programID, "MVP");
	GLint MV_ID =    glGetUniformLocation(programID, "MV");
	GLint View_ID =  glGetUniformLocation(programID, "V");
	GLint Model_ID = glGetUniformLocation(programID, "M");
	GLint LightID =  glGetUniformLocation(programID, "LightPosition_worldspace");

	// Load the texture
	GLuint Texture = loadDDS("playground/uvmap.DDS");
	// Get a handle for our "myTextureSampler" uniform
	GLint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	glm::vec3 lightPos = glm::vec3(-1.5,0,0);

	std::vector<vec3>           star_vertices;
	std::vector<unsigned short> star_indeces;
	std::vector<vec2>           star_uvs;
	std::vector<vec3>           star_normals;
    auto star = Sphere<unsigned  short>(4);
	star.get_viun(star_vertices, star_indeces, star_uvs, star_normals);
	GLuint star_vertex_buffer;
	glGenBuffers(1, &star_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, star_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, star_vertices.size() * sizeof(vec3), &star_vertices[0], GL_STATIC_DRAW);
	GLuint star_index_buffer;
	glGenBuffers(1, &star_index_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, star_index_buffer);
	glBufferData(GL_ARRAY_BUFFER, star_indeces.size() * sizeof(unsigned short), &star_indeces[0], GL_STATIC_DRAW);
	GLuint star_uv_buffer;
	glGenBuffers(1, &star_uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, star_uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, star_uvs.size() * sizeof(vec2), &star_uvs[0], GL_STATIC_DRAW);
	GLuint star_normal_buffer;
	glGenBuffers(1, &star_normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, star_normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, star_normals.size() * sizeof(vec3), &star_normals[0], GL_STATIC_DRAW);

	std::vector<vec3>           disk_vertices;
	std::vector<unsigned short> disk_indeces;
	std::vector<vec2>           disk_uvs;
	std::vector<vec3>           disk_normals;
	auto disk = Circle<unsigned  short>(4);
	disk.get_viun(disk_vertices, disk_indeces, disk_uvs, disk_normals);
	GLuint disk_vertex_buffer;
	glGenBuffers(1, &disk_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, disk_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, disk_vertices.size() * sizeof(vec3), &disk_vertices[0], GL_STATIC_DRAW);
	GLuint disk_index_buffer;
	glGenBuffers(1, &disk_index_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, disk_index_buffer);
	glBufferData(GL_ARRAY_BUFFER, disk_indeces.size() * sizeof(unsigned short), &disk_indeces[0], GL_STATIC_DRAW);
	GLuint disk_uv_buffer;
	glGenBuffers(1, &disk_uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, disk_uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, disk_uvs.size() * sizeof(vec2), &disk_uvs[0], GL_STATIC_DRAW);
	GLuint disk_normal_buffer;
	glGenBuffers(1, &disk_normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, disk_normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, disk_normals.size() * sizeof(vec3), &disk_normals[0], GL_STATIC_DRAW);

	do{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		{
			auto mvp = MVP(3.0f, 1.5f);

			// Send our transformation to the currently bound shader, in the "MVP" uniform
			// This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
			glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, &mvp.mvp()[0][0]);
			glUniformMatrix4fv(MV_ID, 1, GL_FALSE, &mvp.mv()[0][0]);
			glUniformMatrix4fv(View_ID, 1, GL_FALSE, &mvp.getViewMatrix()[0][0]);
			glUniformMatrix4fv(Model_ID, 1, GL_FALSE, &mvp.getModelMatrix()[0][0]);

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			// Set our "myTextureSampler" sampler to user Texture Unit 0
			glUniform1i(TextureID, 0);

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, star_vertex_buffer);
			glVertexAttribPointer(
					0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
					3,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					0,                  // stride
					(void *) 0            // array buffer offset
			);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, star_uv_buffer);
			glVertexAttribPointer(
					1,                                // attribute
					2,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void *) 0                          // array buffer offset
			);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, star_normal_buffer);
			glVertexAttribPointer(
					2,                                // attribute
					3,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void *) 0                          // array buffer offset
			);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, star_index_buffer);
			// Draw the triangles!
			glDrawElements(
					GL_TRIANGLES,             // mode
					(GLsizei) star_indeces.size(), // count
					GL_UNSIGNED_SHORT,        // type
					(void *) 0                  // element array buffer offset
			);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
		}

		{
			auto mvp = MVP(3.0f, -1.5f);

			// Send our transformation to the currently bound shader, in the "MVP" uniform
			// This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
			glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, &mvp.mvp()[0][0]);
			glUniformMatrix4fv(MV_ID, 1, GL_FALSE, &mvp.mv()[0][0]);
			glUniformMatrix4fv(View_ID, 1, GL_FALSE, &mvp.getViewMatrix()[0][0]);
			glUniformMatrix4fv(Model_ID, 1, GL_FALSE, &mvp.getModelMatrix()[0][0]);

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, Texture);
			// Set our "myTextureSampler" sampler to user Texture Unit 0
			glUniform1i(TextureID, 0);

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, disk_vertex_buffer);
			glVertexAttribPointer(
					0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
					3,                  // size
					GL_FLOAT,           // type
					GL_FALSE,           // normalized?
					0,                  // stride
					(void *) 0            // array buffer offset
			);

			// 2nd attribute buffer : UVs
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, disk_uv_buffer);
			glVertexAttribPointer(
					1,                                // attribute
					2,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void *) 0                          // array buffer offset
			);

			// 3rd attribute buffer : normals
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, disk_normal_buffer);
			glVertexAttribPointer(
					2,                                // attribute
					3,                                // size
					GL_FLOAT,                         // type
					GL_FALSE,                         // normalized?
					0,                                // stride
					(void *) 0                          // array buffer offset
			);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, disk_index_buffer);
			// Draw the triangles!
			glDrawElements(
					GL_TRIANGLES,             // mode
					(GLsizei) disk_indeces.size(), // count
					GL_UNSIGNED_SHORT,        // type
					(void *) 0                  // element array buffer offset
			);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the Space key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_SPACE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &star_vertex_buffer);
	glDeleteBuffers(1, &star_uv_buffer);
	glDeleteBuffers(1, &star_normal_buffer);
	glDeleteBuffers(1, &star_index_buffer);
	glDeleteBuffers(1, &disk_vertex_buffer);
	glDeleteBuffers(1, &disk_uv_buffer);
	glDeleteBuffers(1, &disk_normal_buffer);
	glDeleteBuffers(1, &disk_index_buffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

