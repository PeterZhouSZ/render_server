#include <iostream>
#include <string>
#include <cxxopts/cxxopts.hpp>
#include <json/json.h>
#include <npy/npy.hpp>

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

};

class Object {
public:
    Object();


};

class Scene {
public:
    Scene(const std::string& filename) {}
};

class GLShader {

};

class GLProgram {

};

static const struct {
    float x, y;
    float r, g, b;
} vertices[3] = {
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
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
    GLRenderer(const Scene* scene, const std::string& output_dir, int width, int height): 
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
    void render(const Scene* scene);

    // Render the current scene from a different viewpoint
    void render(const Camera& camera);
private:
    std::string mOutputDir;
    const Scene* mScene;
    GLFWwindow* mWindow;
    int mWidth, mHeight;
    GLuint mProgram;
    GLuint vertex_buffer, vertex_shader, fragment_shader;
    GLint mvp_location, vpos_location, vcol_location;
    float ratio;
    mat4x4 mvp;
    char* buffer;

    void init();
    void setupScene();
    void updateCamera(const Camera& camera);
};

void GLRenderer::init() {
    glfwSetErrorCallback(error_callback);

    glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);

    if(!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    mWindow = glfwCreateWindow(640, 480, "Test", NULL, NULL);
    if(!mWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(mWindow);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
}

void GLRenderer::setupScene() {
    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    mProgram = LoadShaders("../scenes/shaders/basic/vs.glsl", "../scenes/shaders/basic/fs.glsl");

    mvp_location = glGetUniformLocation(mProgram, "MVP");
    vpos_location = glGetAttribLocation(mProgram, "vPos");
    vcol_location = glGetAttribLocation(mProgram, "vCol");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) 0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 2));

    glfwGetFramebufferSize(mWindow, &mWidth, &mHeight);
    ratio = mWidth / (float) mHeight;

    glViewport(0, 0, mWidth, mHeight);
    
}

void GLRenderer::render(const Scene* scene) {
    mScene = scene;

    // Setup the resources on the GPU
    setupScene();

    // Render
    render();
}

void GLRenderer::render() {
    mat4x4_ortho(mvp, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    std::cout << mvp_location << std::endl;
    glUseProgram(mProgram);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
    glDrawArrays(GL_TRIANGLES, 0, 3);

#if USE_NATIVE_OSMESA
    glfwGetOSMesaColorBuffer(window, &width, &height, NULL, (void**) &buffer);
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

    std::ifstream ifs(scene_filename);

    Json::CharReaderBuilder reader;
    Json::Value obj;
    std::string json_err;
    Json::parseFromStream(reader, ifs, &obj, &json_err);
    auto camera_params = obj["camera"];
    std::cout << "camera-params " << camera_params << std::endl;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings;
    std::string errors;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials,
    &warnings, &errors, "halfbox.obj", "./");
    std::cout << "ret " << ret << std::endl;
    std::cout << "warnings " << warnings << std::endl;
    std::cout << "errors " << errors << std::endl;
    std::cout << attrib.vertices.size() << std::endl;
    std::cout << attrib.normals.size() << std::endl;
    std::cout << shapes.size() << std::endl;

    Scene scene(scene_filename);
    GLRenderer renderer(&scene, out_dir, 640, 480);
    renderer.render();

    return 0;
}
