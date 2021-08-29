#include "CustomCamera.hpp"
#include "Example.hpp"
#include "CustomWindow.hpp"
#include "core/context/global.hpp"

int main() {
    const int width = 640;
    const int height = 640;

    kuafu::Kuafu renderer;
    renderer.getConfig().setAssetsPath("../resources");

    auto &scene = renderer.getScene();
//    scene.setCamera(std::make_shared<CustomCamera>(
//            width, height, glm::vec3(0.f, 0.f, 3.f)));
    renderer.setWindow(std::make_shared<CustomWindow>(
            width, height, "Test", SDL_WINDOW_RESIZABLE, &scene));
//    renderer.setGui(std::make_shared<CustomGui>(&renderer));
    renderer.init();

    scene.setCamera(std::make_shared<CustomCamera>(
            width, height, glm::vec3(0.f, 0.f, 3.f)));

    loadScene(&renderer, Level::eNew);

//    renderer.getConfig().setPerPixelSampleRate(256);
//    renderer.getConfig().setUseDenoiser(false);

//    for (int i = 0; i < 100; i++)
//        renderer.run();
//
//    auto geometries = renderer.getScene().getGeometries();
//    kuafu::global::materials[geometries[3]->matIndex.front()].d = 0.f;
////    geometries[3]->setMaterial(mat);
////    geometries[3]->initialized = false;
//    renderer.getScene().markGeometriesChanged();
//    renderer.getScene().markGeometryInstancesChanged();

    while (renderer.isRunning()) {
//        updateScene(&renderer);
        renderer.run();
    }

    return 0;
}
