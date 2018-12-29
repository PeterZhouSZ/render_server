#include <iostream>
#include <string>
#include <map>
#include <cxxopts/cxxopts.hpp>
#include <json/json.h>
#include <npy/npy.hpp>
#include <glm/glm.hpp>

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
    virtual void setup(GLuint vpos_location, GLuint vcol_location) {}
    virtual void render() {}
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 albedo;
    glm::vec3 normal;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class Triangle2D : public Object {
public:
    Triangle2D() {
        // constructor only for creating the geometry
        
    }

    void setup(GLuint vpos_location, GLuint vcol_location) override {
        const struct {
            float x, y;
            float r, g, b;
        } vertices[3] = {
            { -0.6f, -0.4f, 1.f, 0.f, 0.f },
            {  0.6f, -0.4f, 0.f, 1.f, 0.f },
            {   0.f,  0.6f, 0.f, 0.f, 1.f }
        };
        // create vao??
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Create VBO
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                            sizeof(vertices[0]), (void*) 0);
        glEnableVertexAttribArray(vcol_location);
        glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                            sizeof(vertices[0]), (void*) (sizeof(float) * 2));
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

class Scene {
public:
    Scene(const std::string& filename) {
        loadScene(filename);void setup(const std::map<std::string, GLuint>& glsl_vars);
    }

    void setup(std::map<std::string, GLuint>& glsl_vars);
    void render();
private:
    void loadScene(const std::string& filename);
    std::vector<Object*> mObjects;
};

void Scene::loadScene(const std::string& filename)
{
    std::ifstream ifs(filename);

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
    &warnings, &errors, "../scenes/objs/halfbox.obj", "./");
    std::cout << "ret " << ret << std::endl;
    std::cout << "warnings " << warnings << std::endl;
    std::cout << "errors " << errors << std::endl;
    std::cout << attrib.vertices.size() << std::endl;
    std::cout << attrib.normals.size() << std::endl;
    std::cout << shapes.size() << std::endl;

    mObjects.push_back(new Triangle2D());
}

void Scene::setup(std::map<std::string, GLuint>& glsl_vars)
{
    for(auto obj: mObjects) {
        obj->setup(glsl_vars["vpos_location"], glsl_vars["vcol_location"]);
    }
}

void Scene::render() {
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
private:
    std::string mOutputDir;
    Scene* mScene;
    GLFWwindow* mWindow;
    int mWidth, mHeight;
    GLuint mProgram;
    //GLuint vertex_buffer, vertex_shader, fragment_shader;
    GLint mvp_location, vpos_location, vcol_location;
    Triangle2D* tri; // move this to scene
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(mWidth, mHeight, "Test", NULL, NULL);
    if(!mWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(mWindow);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
}

void GLRenderer::setupScene() {
    // TODO: MOVE THIS TO SCENE?
    // NOTE: OpenGL error checks have been omitted for brevity
    // scene.vertex_shader_path(), scene.fragment_shader_path()
    mProgram = LoadShaders("../scenes/shaders/basic/vs.glsl", "../scenes/shaders/basic/fs.glsl");

    mvp_location = glGetUniformLocation(mProgram, "MVP");
    vpos_location = glGetAttribLocation(mProgram, "vPos");
    vcol_location = glGetAttribLocation(mProgram, "vCol");
    // scene.init_objects(vpos_location, vcol_location);
    std::map<std::string, GLuint> var_name_map;
    var_name_map["vpos_location"] = vpos_location;
    var_name_map["vcol_location"] = vcol_location;
    mScene->setup(var_name_map);
 
    glfwGetFramebufferSize(mWindow, &mWidth, &mHeight);
    ratio = mWidth / (float) mHeight;

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
    mat4x4_ortho(mvp, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    std::cout << mvp_location << std::endl;
    glUseProgram(mProgram);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
    mScene->render();
    //tri->render();

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
    renderer.render();

    return 0;
}
