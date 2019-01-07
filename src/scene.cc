#include <iostream>
#include <fstream>
#include <map>

#include <glad/glad.h>

#include "utils.h"
#include "object.h"
#include "shader.h"
#include "scene.h"

#include <glm/gtc/type_ptr.hpp>

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

    loadLights(obj["lights"]);
    loadMaterials(obj["materials"]);

    auto objects_specs = obj["objects"];
    std::cout << "objects: " << objects_specs << std::endl;

    //mObjects.push_back(new TestTriangle());
    for(auto obj: objects_specs["obj"]) {
        std::cout << obj["path"].asString() << std::endl;
        glm::vec3 translate(0.0);
        glm::mat4 rotate(1.0);
        if(obj["translate"]) {
            std::cout << obj["translate"] << std::endl;
            translate = glm::vec3(obj["translate"][0].asFloat(), obj["translate"][1].asFloat(), obj["translate"][2].asFloat());
        }
        mObjects.push_back(new TriangleMesh(basedir + "/" + obj["path"].asString(), translate, rotate));
    }
}

void Scene::loadLights(const Json::Value& light_spec)
{
    std::cout << "Lights" << std::endl;
    std::cout << light_spec << std::endl;

    auto ambient = light_spec["ambient"];
    mAmbient = glm::vec3(ambient[0].asFloat(), ambient[1].asFloat(), ambient[2].asFloat());
    
}

void Scene::loadMaterials(const Json::Value& material_spec)
{

}

void Scene::setup()
{
    mProgram = LoadShaders(mVertexShaderPath, mFragmentShaderPath);

    model_matrix_location = glGetUniformLocation(mProgram, "model");
    view_matrix_location = glGetUniformLocation(mProgram, "view");
    projection_matrix_location = glGetUniformLocation(mProgram, "projection");

    position_location = glGetAttribLocation(mProgram, "position");
    normal_location = glGetAttribLocation(mProgram, "normal");
    albedo_location = glGetAttribLocation(mProgram, "albedo");
    coeffs_location = glGetAttribLocation(mProgram, "coeffs");

    ambient_location = glGetUniformLocation(mProgram, "ambient");

    std::map<std::string, GLuint> var_name_map;
    var_name_map["position"] = position_location;
    var_name_map["normal"] = normal_location;
    var_name_map["albedo"] = albedo_location;
    var_name_map["coeffs"] = coeffs_location;
    for(auto obj: mObjects) {
        obj->setup(var_name_map);
    }
}

void Scene::render() {
    glm::mat4 mModel = glm::mat4(1.0);
    glm::mat4 mView = mCamera->getViewMatrix();
    glm::mat4 mProjection = mCamera->getProjectionMatrix();
    glUseProgram(mProgram);
    
    glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE, glm::value_ptr(mView));
    glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, glm::value_ptr(mProjection));
    glUniform3fv(ambient_location, 1, glm::value_ptr(mAmbient));

    for(auto obj: mObjects) {
        glm::mat4 curr_model_tform = mModel * obj->get_transformation();
        glUniformMatrix4fv(model_matrix_location, 1, GL_FALSE, glm::value_ptr(curr_model_tform));
        obj->render(model_matrix_location);
    }
}
