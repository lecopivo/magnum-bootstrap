#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Mesh.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Renderer.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shader.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Trade/MeshData3D.h>

#include <MagnumImGui/MagnumImGui.h>
#include <MagnumImGui/imgui.h>

#include <tuple>

#include "visualization/VisualizationPrimitives.h"

using namespace Magnum;
using namespace Magnum::Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D>  Scene3D;

class MyApplication : public Platform::Application {
public:
  explicit MyApplication(const Arguments &arguments);

private:
  void drawEvent() override;
  void drawGui();

  void viewportEvent(const Vector2i &size) override;

  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void mouseScrollEvent(MouseScrollEvent &event) override;
  void textInputEvent(TextInputEvent &event) override;

  void mouseRotation(MouseMoveEvent const &event, Vector2 delta);
  void mouseZoom(MouseMoveEvent const &event, Vector2 delta);
  void mousePan(MouseMoveEvent const &event, Vector2 delta);

  /*
   * Sends a ray from a camera pixel
   *
   *
   *
   * \param pixel pixel to send a ray from
   * \return direction vector and camera position(in that order)
   */
  std::tuple<Vector3, Vector3> cameraRayCast(Vector2i pixel) const;

  Scene3D                     _scene;
  Object3D *                  _cameraObject;
  SceneGraph::Camera3D *      _camera;
  SceneGraph::DrawableGroup3D _drawables;

  Vector2i _previousMousePosition;
  Vector3  _mousePlanePosition[2];

  MagnumImGui _gui;

  // Example objects to draw
  DrawablePlane * plane;
  DrawableSphere *sphere;
  // DrawableLine *  line;
};

MyApplication::MyApplication(const Arguments &arguments)
    : Platform::Application{
          arguments,
          Configuration{}
              .setTitle("Magnum object picking example")
              .setWindowFlags(
                  Sdl2Application::Configuration::WindowFlag::Resizable)} {
  /* Configure OpenGL state */
  Renderer::enable(Renderer::Feature::DepthTest);
  Renderer::enable(Renderer::Feature::FaceCulling);

  /* Configure camera */
  _cameraObject = new Object3D{&_scene};
  _cameraObject->translate(Vector3::zAxis(4.0f)).rotateX(Rad{M_PI / 4});
  _camera = new SceneGraph::Camera3D{*_cameraObject};
  viewportEvent(defaultFramebuffer.viewport().size()); // set up camera

  /* Set up object to draw */
  plane  = (new DrawablePlane(&_scene, &_drawables, 10, 10));
  sphere = (new DrawableSphere(&_scene, &_drawables, 10, 10));
  // line   = (new DrawableLine(&_scene, &_drawables, 10));

  sphere->setVertices([](int i, DrawableMesh::VertexData &v) {
    v.color = Color4::fromHsv(Rad{2.0f * M_PI * i / 100.0f}, 1.0, 1.0);
  });
}

void MyApplication::drawEvent() {
  defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

  _camera->draw(_drawables);

  drawGui();

  swapBuffers();
}

void MyApplication::drawGui() {
  _gui.newFrame(windowSize(), defaultFramebuffer.viewport().size());

  // ImGui::ColorEdit3("Box color", &(_color[0]));
  ImGui::Text("asdfasdf");

  if (ImGui::Button("Vertex color shader")) {
    sphere->_shader = Shaders::VertexColor3D{};
  }

  if (ImGui::Button("Phong shader")) {
    sphere->_shader = Shaders::Phong{};
    auto &shader    = std::get<Shaders::Phong>(sphere->_shader);
    shader.setDiffuseColor(Color4{0.4f, 0.4f, 0.8f, 1.f})
        .setAmbientColor(Color3{0.25f, 0.2f, 0.23f});
  }

  _gui.drawFrame();

  redraw();
}

void MyApplication::viewportEvent(const Vector2i &size) {
  defaultFramebuffer.setViewport({{}, size});

  _camera->setProjectionMatrix(Matrix4::perspectiveProjection(
      60.0_degf, Vector2{size}.aspectRatio(), 0.001f, 10000.0f));
}

void MyApplication::keyPressEvent(KeyEvent &event) {
  if (_gui.keyPressEvent(event)) {
    redraw();
    return;
  }

  if (event.key() == KeyEvent::Key::Esc) {
    exit();
  }

  redraw();
}

void MyApplication::keyReleaseEvent(KeyEvent &event) {
  if (_gui.keyReleaseEvent(event)) {
    redraw();
    return;
  }
}

void MyApplication::mousePressEvent(MouseEvent &event) {
  if (_gui.mousePressEvent(event)) {
    redraw();
    return;
  }

  if (event.button() == MouseEvent::Button::Left) {
    _previousMousePosition = event.position();
    event.setAccepted();
  }
}

void MyApplication::mouseReleaseEvent(MouseEvent &event) {
  if (_gui.mouseReleaseEvent(event)) {
    redraw();
    return;
  }

  event.setAccepted();
  redraw();
}

void MyApplication::mouseMoveEvent(MouseMoveEvent &event) {
  if (_gui.mouseMoveEvent(event)) {
    redraw();
    return;
  }

  const Vector2 delta = Vector2{event.position() - _previousMousePosition} /
                        Vector2{defaultFramebuffer.viewport().size()};

  if ((event.modifiers() & MouseMoveEvent::Modifier::Alt) &&
      (event.buttons() & MouseMoveEvent::Button::Left))
    mouseRotation(event, delta);

  if (event.modifiers() & MouseMoveEvent::Modifier::Alt &&
      event.buttons() & MouseMoveEvent::Button::Right)
    mouseZoom(event, delta);

  if (event.modifiers() & MouseMoveEvent::Modifier::Alt &&
      event.buttons() & MouseMoveEvent::Button::Middle)
    mousePan(event, delta);

  _previousMousePosition = event.position();
  event.setAccepted();
  redraw();
}

void MyApplication::mouseScrollEvent(MouseScrollEvent &event) {
  if (_gui.mouseScrollEvent(event)) {
    redraw();
    return;
  }
}

void MyApplication::textInputEvent(TextInputEvent &event) {
  if (_gui.textInputEvent(event)) {
    redraw();
    return;
  }
}

void MyApplication::mouseRotation(MouseMoveEvent const &event, Vector2 delta) {

  auto camPos =
      _cameraObject->transformation().transformPoint(Vector3{0.0, 0.0, 0.0});

  auto axis = cross(Vector3{0.f, 0.f, 1.f}, camPos.normalized()).normalized();

  _cameraObject->rotate(Rad{-3.0f * delta.y()}, axis);
  _cameraObject->rotateZ(Rad{-3.0f * delta.x()});
}

void MyApplication::mouseZoom(MouseMoveEvent const &event, Vector2 delta) {
  auto dir =
      _cameraObject->transformation().transformVector(Vector3{0.0, 0.0, 1.0});

  _cameraObject->translate(10.0f * delta.y() * dir);
}

void MyApplication::mousePan(MouseMoveEvent const &event, Vector2 delta) {}

std::tuple<Vector3, Vector3>
MyApplication::cameraRayCast(Vector2i pixel) const {

  Vector2 mouseScreenPos =
      2.0f * (Vector2{pixel} / Vector2{defaultFramebuffer.viewport().size()} -
              Vector2{.5f, 0.5f});
  mouseScreenPos[1] *= -1.f;

  Vector3 dir = {mouseScreenPos[0], mouseScreenPos[1], -1.f};
  auto    trans =
      _cameraObject->transformation() * _camera->projectionMatrix().inverted();
  dir = trans.transformVector(dir);

  Vector3 camPos =
      _cameraObject->transformation().transformPoint(Vector3{0.f, 0.f, 0.f});

  return {dir, camPos};
}

MAGNUM_APPLICATION_MAIN(MyApplication)
