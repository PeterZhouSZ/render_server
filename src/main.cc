#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cxxopts/cxxopts.hpp>
#include <json/json.h>
#include <npy/npy.hpp>

#include "light.h"
#include "utils.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#if USE_NATIVE_OSMESA
    #define GLFW_EXPOSE_NATIVE_OSMESA
    #include <GLFW/glfw3native.h>
#endif

#include <linmath.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <tiny_obj_loader.h>


std::string load_shader_code(const std::string& path)
{
    std::string code = "";
    std::ifstream shader_file(path.c_str(), std::ios::in);
    if(shader_file.is_open()) {
        std::stringstream ss;
        ss << shader_file.rdbuf();
        code = ss.str();
    } else {
        std::cout << "Error: Unable to read " << path << std::endl;
    }
    return code;
}

GLuint compile_shader(const std::string& shader,
    GLuint shader_id) 
{
    char const * ptr = shader.c_str();
    glShaderSource(shader_id, 1, &ptr, NULL);
    glCompileShader(shader_id);

    GLint res = GL_FALSE;
    int infoLogLength;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &res);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {

    }
    return res;
}

GLuint LoadShaders(const std::string& vertex_file_path,
    const std::string& fragment_file_path)
{
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vs_code = load_shader_code(vertex_file_path);
    std::string fs_code = load_shader_code(fragment_file_path);

    // Compile the shader
    compile_shader(vs_code, VertexShaderID);
    compile_shader(fs_code, FragmentShaderID);

    GLuint progID = glCreateProgram();
    glAttachShader(progID, VertexShaderID);
    glAttachShader(progID, FragmentShaderID);
    glLinkProgram(progID);

    // Check
    //...

    glDetachShader(progID, VertexShaderID);
    glDetachShader(progID, FragmentShaderID);

    return progID;
}

class Camera {
public:
    Camera(const Json::Value& camera_spec);
    Camera(glm::vec3 pos, glm::vec3 lookat, glm::vec3 up, float focal_length, float fovy);
    Camera(float focal_length, float fovy);

    void setCameraParameters(glm::vec3 pos, glm::vec3 lookat, glm::vec3 up, float focal_length, float fovy);

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    glm::mat4 getViewProjectionMatrix();

    float getAspectRatio() { return getWidth() / getHeight(); }
    float getWidth() { return mViewport[2] - mViewport[0]; }
    float getHeight() { return mViewport[3] - mViewport[1]; }

    std::string str() const;
private:
    glm::vec3 mPos;
    glm::vec3 mUp;
    glm::vec3 mAt;
    glm::vec3 mWorldUp;
    float mFovy;
    float mFocalLength;
    float mNear, mFar;
    float mViewport[4];
};

std::string Camera::str() const {
    std::stringstream ss;
    ss << "Camera configuration:\n";
    ss << "eye : [" << mPos[0] << "," << mPos[1] << "," << mPos[2] << "]\n";
    ss << "at  : [" << mAt[0] << "," << mAt[1] << "," << mAt[2] << "]\n";
    ss << "up  : [" << mUp[0] << "," << mUp[1] << "," << mUp[2] << "]\n";
    ss << "near: " << mNear << "\n";
    ss << "far : " << mFar << "\n";
    ss << "fovy: " << mFovy << "\n";
    ss << "focal_length: " << mFocalLength << "\n";
    ss << "viewport  : [" << mViewport[0] << "," << mViewport[1] << "," << mViewport[2] << "," << mViewport[3] << "]\n";
    ss << "\n";
    return ss.str();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(mPos, mAt, mUp);
}

glm::mat4 Camera::getProjectionMatrix() {
    return glm::perspective(mFovy, getAspectRatio(), mNear, mFar);
}

glm::mat4 Camera::getViewProjectionMatrix() {
    return getProjectionMatrix() * getViewMatrix();
}

Camera::Camera(const Json::Value& camera_spec) {
    mPos = glm::vec3(camera_spec["eye"][0].asFloat(), camera_spec["eye"][1].asFloat(), camera_spec["eye"][2].asFloat());
    mAt = glm::vec3(camera_spec["at"][0].asFloat(), camera_spec["at"][1].asFloat(), camera_spec["at"][2].asFloat());
    mUp = glm::vec3(camera_spec["up"][0].asFloat(), camera_spec["up"][1].asFloat(), camera_spec["up"][2].asFloat());
    mFovy = camera_spec["fovy"].asFloat();
    mFocalLength = camera_spec["focal_length"].asFloat();
    mNear = camera_spec["near"].asFloat();
    mFar = camera_spec["far"].asFloat();

    for(int i = 0; i < 4; i++) {
        mViewport[i] = camera_spec["viewport"][i].asFloat();
    }

    std::cout << str() << std::endl;
}

struct Object {

};

using GLSLVarMap = std::map<std::string, GLuint>;

class GLRenderableObject: public Object {
    // Objects that can be rendered using OpenGL
public:
    virtual void setup(GLSLVarMap& var_map) = 0;
    virtual void render() = 0;
};

struct Material {
    glm::vec3 albedo;
    glm::vec3 coeffs; // kd, ks, theta
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    Material mat;
};

class TestTriangle : public GLRenderableObject {
public:
    TestTriangle() {
        // constructor only for creating the geometry
    }

    void setup(GLSLVarMap& var_map) override {
        GLuint vpos_location = var_map["position"];
        GLuint vcol_location = var_map["albedo"];
        const struct {
            float x, y, z;
            float r, g, b;
        } vertices[3] = {
            { -0.6f, -0.4f, 0.0f, 1.f, 0.f, 0.f },
            {  0.6f, -0.4f, 0.0f, 0.f, 1.f, 0.f },
            {   0.f,  0.6f, 0.0f, 0.f, 0.f, 1.f }
        };
        // create vao??
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Create VBO
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
                            sizeof(vertices[0]), (void*) 0);
        glEnableVertexAttribArray(vcol_location);
        glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                            sizeof(vertices[0]), (void*) (sizeof(float) * 3));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void render() override {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

private:
    GLuint vbo;
    GLuint vao;
};

class Triangle {
    // A triangle face
public:
    Triangle(glm::vec4 vertices[3], glm::vec4 normal);

private:
    glm::vec4 mVertices[3];
    glm::vec4 normal;
};

class TriangleMesh: public GLRenderableObject {
    // Renderable triangle mesh
    // List of vertices and faces of triangles
public:
    TriangleMesh(const std::string& obj_filename) {
        loadObj(obj_filename);
    }

    void setup(GLSLVarMap& var_map) override;
    void render() override;
private:
    GLuint mVao;
    GLuint mVbo;
    GLuint mIbo;
    std::vector<float> mBuffer;
    std::vector<glm::vec3> mVertices;
    std::vector<glm::ivec3> mIndices;   // vertex indices forming a face

    void loadObj(const std::string& filename);
};

void TriangleMesh::loadObj(const std::string& filename)
{
    std::string basedir = get_basedir(filename);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings;
    std::string errors;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials,
    &warnings, &errors, filename.c_str(), basedir.c_str());
    std::cout << "ret " << ret << std::endl;
    std::cout << "warnings " << warnings << std::endl;
    std::cout << "errors " << errors << std::endl;
    std::cout << "num vertices: " << attrib.vertices.size() << std::endl;
    std::cout << "num normals: " << attrib.normals.size() << std::endl;
    std::cout << "num shapes: " << shapes.size() << std::endl;

    std::vector<glm::vec3> raw_vertices;
    for(size_t i = 0; i < attrib.vertices.size() / 3; i++) {
        raw_vertices.push_back({attrib.vertices[i * 3], attrib.vertices[i * 3 + 1], attrib.vertices[i * 3 + 2]});
    }

    for(size_t i = 0; i < shapes.size(); i++) {
        std::cout << "shapes[i].mesh.indices.size() " << shapes[i].mesh.indices.size() << std::endl;
        for(size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
            auto idx = &shapes[i].mesh.indices[3 * f];
            //std::cout << idx[0].vertex_index << " " << idx[1].vertex_index << " " << idx[2].vertex_index << std::endl;
            mIndices.push_back({idx[0].vertex_index, idx[1].vertex_index, idx[2].vertex_index});
            mVertices.push_back(raw_vertices[idx[0].vertex_index]);
            mVertices.push_back(raw_vertices[idx[1].vertex_index]);
            mVertices.push_back(raw_vertices[idx[2].vertex_index]);
            for(int k = 0; k < 3; k++) {
                for(int j = 0; j < 3; j++) {
                    std::cout << raw_vertices[idx[k].vertex_index][j] << " ";
                    mBuffer.push_back(raw_vertices[idx[k].vertex_index][j]);
                }
            }
         }
    }
    std::cout << __FUNCTION__ << " mVertices.size() " << mVertices.size() << std::endl;
    for(auto v: mVertices) {
        std::cout << v[0] << " " << v[1] << " " << v[2] << std::endl;
    }
}

void TriangleMesh::setup(GLSLVarMap& var_map) {
    GLuint vpos_location = var_map["position"];
    GLuint albedo_location = var_map["albedo"];
    std::cout << "mVertices.size() " << mVertices.size() << std::endl;
    std::cout << "mIndices.size() " << mIndices.size() << std::endl;
    size_t buffer_size = mBuffer.size() * sizeof(float); //mVertices.size() * 3 * sizeof(float);
    std::cout << "buffer_size : " << buffer_size << std::endl;

    glGenVertexArrays(1, &mVao);
    glBindVertexArray(mVao);

    glGenBuffers(1, &mVbo);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);

    glBufferData(GL_ARRAY_BUFFER, buffer_size, mBuffer.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
                          0, (void*) 0);
    // glEnableVertexAttribArray(vcol_location);
    // glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
    //                     sizeof(vertices[0]), (void*) (sizeof(float) * 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void TriangleMesh::render()
{
    glBindVertexArray(mVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

class Scene {
public:
    Scene(const std::string& filename) {
        loadScene(filename);
    }

    void setup();
    void render();
private:
    void loadScene(const std::string& filename);
    void loadLights(const Json::Value& light_spec);

    std::vector<GLRenderableObject*> mObjects;
    std::vector<std::shared_ptr<Light>> mLights;
    Camera* mCamera;
    GLuint mProgram;
    GLint model_matrix_location, view_matrix_location, projection_matrix_location;
    GLint position_location, albedo_location;
    std::string mVertexShaderPath;
    std::string mFragmentShaderPath;
};

void Scene::loadScene(const std::string& filename)
{
    std::ifstream ifs(filename);
    std::string basedir = get_basedir(filename);

    std::cout << "filename: " << filename << std::endl;
    Json::CharReaderBuilder reader;
    Json::Value obj;
    std::string json_err;
    Json::parseFromStream(reader, ifs, &obj, &json_err);

    mVertexShaderPath = basedir + "/" + obj["glsl"]["vertex"].asString();
    mFragmentShaderPath = basedir + "/" + obj["glsl"]["fragment"].asString();

    std::cout << "Vertex shader path: " << mVertexShaderPath << std::endl;
    std::cout << "Fragment shader path: " << mFragmentShaderPath << std::endl;

    mCamera = new Camera(obj["camera"]);

    auto objects_specs = obj["objects"];
    std::cout << "objects: " << objects_specs << std::endl;

    //mObjects.push_back(new TestTriangle());
    for(auto obj: objects_specs["obj"]) {
        std::cout << obj["path"].asString() << std::endl;
        mObjects.push_back(new TriangleMesh(basedir + "/" + obj["path"].asString()));
    }
}

void Scene::setup()
{
    mProgram = LoadShaders(mVertexShaderPath, mFragmentShaderPath);

    model_matrix_location = glGetUniformLocation(mProgram, "model");
    view_matrix_location = glGetUniformLocation(mProgram, "view");
    projection_matrix_location = glGetUniformLocation(mProgram, "projection");

    position_location = glGetAttribLocation(mProgram, "position");
    albedo_location = glGetAttribLocation(mProgram, "albedo");

    std::map<std::string, GLuint> var_name_map;
    var_name_map["position"] = position_location;
    var_name_map["albedo"] = albedo_location;
    for(auto obj: mObjects) {
        obj->setup(var_name_map);
    }
}

void Scene::render() {
    glm::mat4 mModel = glm::mat4(1.0);
    glm::mat4 mView = mCamera->getViewMatrix();
    glm::mat4 mProjection = mCamera->getProjectionMatrix();
    glUseProgram(mProgram);
    glUniformMatrix4fv(model_matrix_location, 1, GL_FALSE, glm::value_ptr(mModel));
    glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, glm::value_ptr(mView));
    glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, glm::value_ptr(mProjection));
    for(auto obj: mObjects) {
        obj->render();
    }
}
class GLShader {

};

class GLProgram {

};

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


class GLRenderer {
public:
    GLRenderer(const std::string& output_dir, int width, int height): mOutputDir(output_dir),
    mWidth(width), mHeight(mHeight) {
        init();
    }
    GLRenderer(Scene* scene, const std::string& output_dir, int width, int height): 
        mScene(scene), mOutputDir(output_dir), mWidth(width), mHeight(height) 
        {
            init();
            setupScene();
        }
    ~GLRenderer() {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }

    // Default rendering
    void render();

    // Render a given scene. Useful when the scene is updated
    void render(Scene* scene);

    // Render the current scene from a different viewpoint
    void render(Camera& camera);

    int shouldClose() { return glfwWindowShouldClose(mWindow); }
    void swapBuffers() { glfwSwapBuffers(mWindow);  }

    static void key_callback(GLFWwindow* window,
        int key, int scancode, int action,
        int mods);
private:
    std::string mOutputDir;
    Scene* mScene;
    GLFWwindow* mWindow;
    int mWidth, mHeight;
    char* buffer;

    void init();
    void setupScene();
    void updateCamera(const Camera& camera);
};

void GLRenderer::key_callback(GLFWwindow* window, int key,
			 int scancode, int action,
			 int mods) {
  if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void GLRenderer::init() {
    glfwSetErrorCallback(error_callback);

    glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);

    if(!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(mWidth, mHeight, "Render Server", NULL, NULL);

    if(!mWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(mWindow);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetKeyCallback(mWindow, key_callback);
    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
}

void GLRenderer::setupScene() {
    glfwGetFramebufferSize(mWindow, &mWidth, &mHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    //ratio = mWidth / (float) mHeight;
    mScene->setup();
    glViewport(0, 0, mWidth, mHeight);
}

void GLRenderer::render(Scene* scene) {
    mScene = scene;

    // Setup the resources on the GPU
    setupScene();

    // Render
    render();
}

void GLRenderer::render() {
    /**
     * Activate shader program
     * set camera and global transformations
     * for each object
     *   set transformation
     *   obj.render()
     * Write buffer to file
     */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mScene->render();

#if USE_NATIVE_OSMESA
    glfwGetOSMesaColorBuffer(mWindow, &mWidth, &mHeight, NULL, (void**) &buffer);
#else
    buffer = static_cast<char*>(calloc(4, mWidth * mHeight));
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
#endif

    // Write image Y-flipped because OpenGL
    stbi_write_png("offscreen.png",
                   mWidth, mHeight, 4,
                   buffer + (mWidth * 4 * (mHeight - 1)),
                   -mWidth * 4);
}

void GLRenderer::updateCamera(const Camera& camera) {

}

void interactive_session() {

}

void offline_view_generation_session() {
    /**
     * Randomly generate views, render to offscreen buffer and output to file.
     */
}

int main(int argc, char** argv) {
    std::cout << "Render Server\nBuild date: " << __DATE__ << " " << __TIME__ << "\n" << std::endl;
    cxxopts::Options options("Render", "Render Server");
    options.add_options()
    ("s,scene", "Scene specification json file", cxxopts::value<std::string>())
    ("o,output-dir", "Output directory", cxxopts::value<std::string>());

    auto args = options.parse(argc, argv);
    std::string scene_filename = args["scene"].as<std::string>();
    std::string out_dir = args["output-dir"].as<std::string>();

    std::cout << "Using scene file: " << scene_filename << std::endl;
    std::cout << "Output directory: " << out_dir << std::endl;

    Scene scene(scene_filename);
    GLRenderer renderer(&scene, out_dir, 640, 480); // remove specifying width, height? where should this be specified?
    // if in interactive mode
    while(!renderer.shouldClose()) {
        glfwPollEvents();
        renderer.render();
        renderer.swapBuffers();
    }

    return 0;
}
