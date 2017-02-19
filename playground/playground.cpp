#include <stdio.h>
#include <iostream>
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


double ScrollXOffset = 0.;
double ScrollYOffset = 0.;
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    ScrollXOffset += xoffset;
    ScrollYOffset += yoffset;
}


class MVP{
protected:
	const float scale0;
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
    MVP(float scale): scale0(scale), scale(scale){
        glfwSetScrollCallback(window, ScrollCallback);
		positionFromInput();
    };

	glm::mat4 getProjectionMatrix(){
		return glm::ortho(
				-scale,scale,
				-scale,scale,
				0.0f,100.0f
		);
	}

	glm::mat4 getViewMatrix(glm::vec2 spherical_coordinates){
		return glm::lookAt(
				scale * glm::euclidean(spherical_coordinates),
				glm::vec3(0,0,0), // and looks at the origin
				glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
		);
	}

	glm::mat4 getViewMatrix(){
		return getViewMatrix(glm::vec2(lat, lon));
	}

	glm::mat4 getModelMatrix(){
		return glm::mat4(1.0f);
	}

	glm::mat4 mv(){
		return getViewMatrix() * getModelMatrix();
	}

	glm::mat4 mvp(){
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
	glEnable(GL_CULL_FACE);

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


	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	loadOBJ("playground/suzanne.obj", vertices, uvs, normals);

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	do{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		auto mvp_ = MVP(3.0f);
		auto mvp = mvp_.mvp();

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		// This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
		glUniformMatrix4fv(MVP_ID,   1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(MV_ID,    1, GL_FALSE, &mvp_.mv() [0][0]);
		glUniformMatrix4fv(View_ID,  1, GL_FALSE, &mvp_.getViewMatrix()[0][0]);
		glUniformMatrix4fv(Model_ID, 1, GL_FALSE, &mvp_.getModelMatrix()[0][0]);

		// Light source
		glm::vec3 lightPos = glm::vec3(4,4,4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
		);

        // Draw the triangles !
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei) vertices.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the Space key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_SPACE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

