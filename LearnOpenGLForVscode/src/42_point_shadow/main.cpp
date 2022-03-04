#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <map>

#include <tool/shader.h>
#include <tool/camera.h>
#include <geometry/BoxGeometry.h>
#include <geometry/PlaneGeometry.h>
#include <geometry/SphereGeometry.h>

#define STB_IMAGE_IMPLEMENTATION
#include <tool/stb_image.h>

#include <tool/gui.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);
unsigned int loadTexture(char const *path);

// method
void drawMesh(BufferGeometry geometry);
void drawLightObject(Shader shader, BufferGeometry geometry, glm::vec3 position);

std::string Shader::dirName;

// int SCREEN_WIDTH = 800;
// int SCREEN_HEIGHT = 600;
int SCREEN_WIDTH = 1600;
int SCREEN_HEIGHT = 1200;

// camera value
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// delta time
float deltaTime = 0.0f;
float lastTime = 0.0f;

float lastX = SCREEN_WIDTH / 2.0f; // 鼠标上一帧的位置
float lastY = SCREEN_HEIGHT / 2.0f;

Camera camera(glm::vec3(0.0, 1.0, 6.0));

using namespace std;

int main(int argc, char *argv[])
{
  Shader::dirName = argv[1];
  glfwInit();
  // 设置主要和次要版本
  const char *glsl_version = "#version 330";

  // 片段着色器将作用域每一个采样点（采用4倍抗锯齿，则每个像素有4个片段（四个采样点））
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // 窗口对象
  GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // -----------------------
  // 创建imgui上下文
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // 设置样式
  ImGui::StyleColorsDark();
  // 设置平台和渲染器
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // -----------------------

  // 设置视口
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glEnable(GL_PROGRAM_POINT_SIZE);

  // 启用混合
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 启用gamma校正
  // glEnable(GL_FRAMEBUFFER_SRGB);

  // 深度测试
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // 鼠标键盘事件
  // 1.注册窗口变化监听
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // 2.鼠标事件
  glfwSetCursorPosCallback(window, mouse_callback);
  // 3.将鼠标隐藏
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  Shader sceneShader("./shader/shadow_scene_vert.glsl", "./shader/shadow_scene_frag.glsl");
  Shader depthMapShader("./shader/depth_map_vert.glsl", "./shader/depth_map_frag.glsl", "./shader/depth_map_geo_glsl");
  Shader lightObjectShader("./shader/light_object_vert.glsl", "./shader/light_object_frag.glsl");

  PlaneGeometry groundGeometry(10.0, 10.0);            // 地面
  PlaneGeometry quadGeometry(6.0, 6.0);                // 测试面板
  BoxGeometry boxGeometry(1.0, 1.0, 1.0);              // 箱子
  BoxGeometry floorGeometry(10.0, 0.0001, 10.0);       // 箱子
  SphereGeometry pointLightGeometry(0.06, 10.0, 10.0); // 点光源位置显示

  unsigned int woodMap = loadTexture("./static/texture/wood.png");           // 地面
  unsigned int brickMap = loadTexture("./static/texture/brick_diffuse.jpg"); // 砖墙

  float factor = 0.0;

  // ------------------------------------------------
  unsigned int depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);

  unsigned int depthCubemap;
  glGenTextures(1, &depthCubemap);

  const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
  glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
  for (unsigned int i = 0; i < 6; ++i)
  {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // ------------------------------------------------

  // 定义是个不同的箱子位置
  vector<glm::vec3> cubePositions{
      glm::vec3(2.3f, -2.0f, -1.0),
      glm::vec3(2.0f, 2.3f, 1.0),
      glm::vec3(-2.5f, -1.0f, 0.0),
      glm::vec3(-1.5f, 1.0f, 1.5),
      glm::vec3(-1.5f, 2.0f, -2.5)};

  sceneShader.use();
  sceneShader.setInt("diffuseTexture", 0);
  sceneShader.setInt("depthMap", 1);

  glm::vec3 lightPosition = glm::vec3(-2.0f, 0.0f, 0.0f); // 光照位置
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastTime;
    lastTime = currentFrame;

    // 在标题中显示帧率信息
    // *************************************************************************
    int fps_value = (int)round(ImGui::GetIO().Framerate);
    int ms_value = (int)round(1000.0f / ImGui::GetIO().Framerate);

    std::string FPS = std::to_string(fps_value);
    std::string ms = std::to_string(ms_value);
    std::string newTitle = "LearnOpenGL - " + ms + " ms/frame " + FPS;
    glfwSetWindowTitle(window, newTitle.c_str());

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // *************************************************************************

    // 渲染指令
    // ...
    glClearColor(25.0 / 255.0, 25.0 / 255.0, 25.0 / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lightPosition = glm::vec3(lightPosition.x + glm::sin(glfwGetTime()) * 0.01, lightPosition.y, lightPosition.z);
    glm::mat4 model = glm::mat4(1.0f);

    // ++++++++++++++++++++++++++++++++++++++++++++++++ 渲染深度贴图
    float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
    float near = 1.0f;
    float far = 25.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

    std::vector<glm::mat4> shadowTransforms;
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthMapShader.use();

    for (unsigned int i = 0; i < 6; ++i)
    {
      depthMapShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
    }
    depthMapShader.setFloat("far_plane", far);
    depthMapShader.setVec3("lightPos", lightPosition);

    model = glm::scale(model, glm::vec3(7, 7, 7));
    depthMapShader.setMat4("model", model);
    // 绘制大箱子
    drawMesh(boxGeometry);

    // 绘制多个箱子
    for (unsigned int i = 0; i < cubePositions.size(); i++)
    {
      model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 10.0f * i;
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

      depthMapShader.setMat4("model", model);
      drawMesh(boxGeometry);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // ++++++++++++++++++++++++++++++++++++++++++++++++ 渲染深度贴图

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    sceneShader.use();
    sceneShader.setMat4("projection", projection);
    sceneShader.setMat4("view", view);

    sceneShader.setVec3("lightPos", lightPosition); // 光源位置
    sceneShader.setVec3("viewPos", camera.Position);

    sceneShader.setFloat("far_plane", far);
    sceneShader.setFloat("uvScale", 4.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodMap);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
    model = glm::scale(model, glm::vec3(7, 7, 7));

    sceneShader.setMat4("model", model);
    sceneShader.setFloat("uvScale", 1.0f);
    sceneShader.setInt("reverse_normal", -1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    drawMesh(boxGeometry); //绘制外面的大箱子
    glDisable(GL_CULL_FACE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brickMap);
    // 绘制多个箱子
    for (unsigned int i = 0; i < cubePositions.size(); i++)
    {
      model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 10.0f * i;
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

      sceneShader.setMat4("model", model);
      sceneShader.setInt("reverse_normal", 1);
      drawMesh(boxGeometry);
    }

    // 显示深度贴图
    // *************************************************
    // quadShader.use();
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, depthMap);
    // quadShader.setMat4("view", view);

    // model = glm::mat4(1.0f);
    // quadShader.setFloat("near_plane", near_plane);
    // quadShader.setFloat("far_plane", far_plane);
    // quadShader.setMat4("model", model);
    // quadShader.setMat4("projection", projection);
    // drawMesh(quadGeometry);
    // *************************************************

    drawLightObject(lightObjectShader, pointLightGeometry, lightPosition);

    // 渲染 gui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  groundGeometry.dispose();
  pointLightGeometry.dispose();
  glfwTerminate();

  return 0;
}

// 绘制物体
void drawMesh(BufferGeometry geometry)
{
  glBindVertexArray(geometry.VAO);
  glDrawElements(GL_TRIANGLES, geometry.indices.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

// 绘制灯光物体
void drawLightObject(Shader shader, BufferGeometry geometry, glm::vec3 position)
{
  glm::mat4 view = camera.GetViewMatrix();
  glm::mat4 projection = glm::mat4(1.0f);
  glm::mat4 model = glm::mat4(1.0f);
  projection = glm::perspective(glm::radians(camera.Zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

  // // 绘制灯光物体
  shader.use();
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);

  model = glm::mat4(1.0f);
  model = glm::translate(model, position);

  shader.setMat4("model", model);
  shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
  drawMesh(geometry);
}

// 窗口变动监听
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
}

// 键盘输入监听
void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  // 相机按键控制
  // 相机移动
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(FORWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(LEFT, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    camera.ProcessKeyboard(RIGHT, deltaTime);
  }
}

// 鼠标移动监听
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

// 加载纹理贴图
unsigned int loadTexture(char const *path)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);

  // 图像y轴翻转
  stbi_set_flip_vertically_on_load(true);
  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  if (data)
  {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}