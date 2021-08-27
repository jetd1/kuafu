#include "kuafu.hpp"

enum class Level {
    eCornell,
    eAnimations,
    eSpheres,
    eMirrors,
    eSponza,
    eNew
};

inline Level currentLevel;

inline auto getRandomUniquePosition(float min, float max) -> glm::vec3 {
    static std::vector<glm::vec3> positions;

    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(min, max);

    glm::vec3 result = glm::vec3(0.0F);

    while (true) {
        result.x = dist(mt);
        result.y = dist(mt);
        result.z = dist(mt);

        bool accepted = true;
        for (const auto &position : positions) {
            if (result == position) {
                accepted = false;
            }
        }

        if (accepted) {
            break;
        }
    };

    return result;
}

inline auto getRandomFloat(float min, float max) -> float {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(min, max);

    return dist(mt);
}

inline void addModel(kuafu::Kuafu *renderer, std::string_view path, glm::mat4 transform = glm::mat4(1.0F)) {
    auto model = renderer->getScene().findGeometry(path);
    if (model == nullptr) {
        model = kuafu::loadObj(path);
        renderer->getScene().submitGeometry(model);
    }

    renderer->getScene().submitGeometryInstance(kuafu::instance(model, transform));
}

inline void addBox(kuafu::Kuafu *renderer) {
    std::string_view path = "models/cube.obj";
    auto cube = renderer->getScene().findGeometry(path);
    if (cube == nullptr) {
        cube = kuafu::loadObj(path);
        renderer->getScene().submitGeometry(cube);
    }

    auto transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.3F, 0.3F, 0.3F));
    transform = glm::rotate(transform, getRandomFloat(0.0F, 360.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    transform = glm::translate(transform, getRandomUniquePosition(-25.0F, 25.0F));

    auto cubeInstance = kuafu::instance(cube, transform);
    renderer->getScene().submitGeometryInstance(cubeInstance);
}

inline void addSphere(kuafu::Kuafu *renderer) {
    std::string_view path = "models/sphere.obj";
    auto sphere = renderer->getScene().findGeometry(path);
    if (sphere == nullptr) {
        sphere = kuafu::loadObj(path);
        renderer->getScene().submitGeometry(sphere);
    }

    auto transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.1F, 0.1F, 0.1F));
    transform = glm::rotate(transform, getRandomFloat(0.0F, 360.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    transform = glm::translate(transform, getRandomUniquePosition(-70.0F, 70.0F));

    auto sphereInstance = kuafu::instance(sphere, transform);
    renderer->getScene().submitGeometryInstance(sphereInstance);
}

inline void loadScene(kuafu::Kuafu *renderer, Level scene) {
    currentLevel = scene;

    renderer->getScene().getCamera()->reset();

    if (scene == Level::eCornell) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(1000); // Will give a warning.
        renderer->getConfig().setGeometryInstanceLimit(10000);
        renderer->getConfig().setTextureLimit(1000); // Will give a warning.
        renderer->getConfig().setClearColor(glm::vec4(0.45F, 0.45F, 0.45F, 0.8F));
        renderer->getConfig().setAccumulatingFrames(true);

        //renderer->getScene( ).getCamera( )->setPosition( glm::vec3( 0.0F, 0.0F, -0.5F ) );
        //renderer->getScene( ).getCamera( )->setFov( 90.0F );

        auto cornell = kuafu::loadObj("models/CornellBox.obj");
        //auto cornell = kuafu::loadObj( "models/Sphere.obj" );

//        auto lightPlane = kuafu::loadObj( "models/plane.obj" );
//        kuafu::Material lightMaterial;
//        lightMaterial.emission = glm::vec3( 10.0F );
//        lightPlane->setMaterial( lightMaterial );

        renderer->getScene().setGeometries({cornell});

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, -1.0F, -1.0F));
        auto cornellInstance = kuafu::instance(cornell, transform);

//        transform               = glm::translate( glm::mat4( 1.0F ), glm::vec3( 0.0F, 80.0F, 0.0F ) );
//        auto lightPlaneInstance = kuafu::instance( lightPlane, transform );

        renderer->getScene().setGeometryInstances({cornellInstance,
//                                                   lightPlaneInstance
        });

        renderer->getScene().removeEnvironmentMap();
    } else if (scene == Level::eAnimations) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(5);
        renderer->getConfig().setGeometryInstanceLimit(4000);
        renderer->getConfig().setTextureLimit(4);
        renderer->getConfig().setAccumulatingFrames(false);
        renderer->getConfig().setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));

        // Load geometries.
        auto scene = kuafu::loadObj("models/scene.obj");
        auto plane = kuafu::loadObj("models/plane.obj");

        // Make a custom material for an emissive surface (light source).
        kuafu::Material customMaterial;
        customMaterial.emission = glm::vec3(1.0F);
        plane->setMaterial(customMaterial);

        // Submit geometries.
        renderer->getScene().setGeometries({scene, plane});

        // Create instances of the geometries.
        auto transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.25F));
        transform = glm::rotate(transform, glm::radians(45.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        transform = glm::translate(transform, glm::vec3(0.0F, -2.0F, 1.0F));

        auto sceneInstance = kuafu::instance(scene, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 80.0F, 0.0F));
        auto planeInstance = kuafu::instance(plane, transform);

        // Submit instances for drawing.
        renderer->getScene().setGeometryInstances({sceneInstance, planeInstance});

        renderer->getScene().removeEnvironmentMap();
    } else if (scene == Level::eSpheres) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(100); // Will give a warning.
        renderer->getConfig().setGeometryInstanceLimit(15000);
        renderer->getConfig().setTextureLimit(100); // Will give a warning.
        renderer->getConfig().setAccumulatingFrames(false);
//        renderer->getConfig().setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
        renderer->getConfig().setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 1.0F));

        renderer->getScene().getCamera()->setPosition(glm::vec3(-12.6F, 1.1F, 19.4F));
        renderer->getScene().getCamera()->setFront(glm::vec3(0.67F, 0.0F, -0.8F));

        auto lightPlane = kuafu::loadObj("models/plane.obj");
        kuafu::Material lightMaterial;
        lightMaterial.emission = glm::vec3(1.0F);
        lightPlane->setMaterial(lightMaterial);

        auto floor = kuafu::loadObj("models/plane.obj");
        kuafu::Material floorMaterial;
        floorMaterial.kd = glm::vec3(1.0F);
        floor->setMaterial(floorMaterial);

        auto sphere = kuafu::loadObj("models/sphere.obj");
        kuafu::Material mat;
        mat.illum = 2;
        mat.kd = glm::vec3(0.2F, 0.4F, 1.0F);
        mat.ns = 0.0F;
        mat.roughness = 1000.;
        mat.ior = 0.0F;
        mat.d = 1.0;
        sphere->setMaterial(mat);

//        auto sphere1 = kuafu::loadObj("models/sphere.obj");
//        mat.kd = glm::vec3(1.0F, 0.8F, 0.0F);
//        mat.ns = 0.0F;
//        mat.ior = 1;
//        mat.illum = 0;
//        mat.d = 0.5;
//        mat.ior = 1.0F;
//        sphere1->setMaterial(mat);

        auto sphere1 = kuafu::loadObj("models/sphere.obj");
        mat.illum = 1;
        mat.kd = glm::vec3(0.2F, 0.4F, 1.0F);
        mat.ns = 128.0F;
        mat.roughness = 0.;
        mat.ior = 1.4F;
        sphere1->setMaterial(mat);

        auto sphere2 = kuafu::loadObj("models/sphere.obj");
        mat.illum = 1;
        mat.kd = glm::vec3(1.0F, 1.0F, 1.0F);
        mat.ns = 128.0F;
        mat.roughness = 0.;
        mat.ior = 1.4F;
        sphere2->setMaterial(mat);

        auto sphere3 = kuafu::loadObj("models/sphere.obj");
        mat.illum = 2;
        mat.kd = glm::vec3(0.2F, 0.4F, 1.0F);
        mat.roughness = 0.02F;
        mat.ior = 1.0F;
        sphere3->setMaterial(mat);

        auto sphere4 = kuafu::loadObj("models/sphere.obj");
        mat.illum = 2;
        mat.roughness = 0.2F;
        mat.ior = 1.0F;
        sphere4->setMaterial(mat);

//        auto sphere5 = kuafu::loadObj("models/sphere.obj");
//        mat.illum = 2;
//        mat.roughness = 0.6F;
//        sphere5->setMaterial(mat);
//
//        auto sphere6 = kuafu::loadObj("models/sphere.obj");
//        mat.illum = 2;
//        mat.roughness = 0.8F;
//        sphere6->setMaterial(mat);
//
//        auto sphere7 = kuafu::loadObj("models/sphere.obj");
//        mat.illum = 2;
//        mat.roughness = 1.0F;
//        sphere7->setMaterial(mat);

        auto glass = kuafu::loadObj("models/plane.obj");
        kuafu::Material glassMaterial;
        glassMaterial.kd = glm::vec3(1.0F);
        glassMaterial.ior = 1.0F;
        glassMaterial.illum = 1;
        glassMaterial.ior = 2.6F;
        glassMaterial.roughness = 1.0F;
        glass->setMaterial(glassMaterial);

        renderer->getScene().setGeometries(
                {
                    lightPlane,
                    floor, sphere,
                 sphere1, sphere2, sphere3, sphere4,
//                 sphere5, sphere6, sphere7,
//                 glass
                });

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 80.0F, 0.0F));
        auto lightPlaneInstance = kuafu::instance(lightPlane, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.2F, 0.2F, 0.2F));
        auto floorInstance = kuafu::instance(floor, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-5.0F, 0.0F, 0.0F));
        transform = glm::scale(transform, glm::vec3(0.5));
        auto sphereInstance = kuafu::instance(sphere, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-2.5F, 0.0F, 0.0F));
        auto sphereInstance1 = kuafu::instance(sphere1, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
        auto sphereInstance2 = kuafu::instance(sphere2, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(2.5F, 0.0F, 0.0F));
        auto sphereInstance3 = kuafu::instance(sphere3, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(5.0F, 0.0F, 0.0F));
        auto sphereInstance4 = kuafu::instance(sphere4, transform);
//
//        transform = glm::translate(glm::mat4(1.0F), glm::vec3(7.5F, 0.0F, 0.0F));
//        auto sphereInstance5 = kuafu::instance(sphere5, transform);
//
//        transform = glm::translate(glm::mat4(1.0F), glm::vec3(10.0F, 0.0F, 0.0F));
//        auto sphereInstance6 = kuafu::instance(sphere6, transform);
//
//        transform = glm::translate(glm::mat4(1.0F), glm::vec3(12.5F, 0.0F, 0.0F));
//        auto sphereInstance7 = kuafu::instance(sphere7, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.05F, 0.05F, 0.05F));
        transform = glm::rotate(transform, glm::radians(90.0F), glm::vec3(1.0F, 0.0F, 0.0F));
        transform = glm::translate(transform, glm::vec3(0.0F, 250.0F, 0.0F));
        auto glassInstance = kuafu::instance(glass, transform);

        renderer->getScene().setGeometryInstances(
                {
                    lightPlaneInstance,
                    floorInstance, sphereInstance,
                 sphereInstance1, sphereInstance2, sphereInstance3,
                 sphereInstance4,
//                 sphereInstance5, sphereInstance6, sphereInstance7,
//                 glassInstance
                });
    } else if (scene == Level::eMirrors) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(3); // Will give a warning.
        renderer->getConfig().setGeometryInstanceLimit(50000);
        renderer->getConfig().setTextureLimit(2); // Will give a warning.

        renderer->getConfig().setAccumulatingFrames(false);
        renderer->getConfig().setClearColor(glm::vec4(0.5F, 0.5F, 0.7F, 1.0F));

        renderer->getScene().getCamera()->setPosition(glm::vec3(9.8F, 0.3F, 7.7F));
        renderer->getScene().getCamera()->setFront(glm::vec3(-0.5F, 0.0F, -0.8F));
        //renderer->getScene( ).getCamera( )->setFront( glm::vec3( -0.5F, 0.0F, -0.8F ) );
        renderer->getScene().removeEnvironmentMap();

        auto lightPlane = kuafu::loadObj("models/plane.obj");
        kuafu::Material lightMaterial;
        lightMaterial.emission = glm::vec3(1.0F);
        lightPlane->setMaterial(lightMaterial);

        auto mirrorPlane = kuafu::loadObj("models/plane.obj");
        kuafu::Material mirrorMaterial;
        mirrorMaterial.illum = 2;
        mirrorMaterial.roughness = 0.0F;
        mirrorMaterial.kd = glm::vec3(0.95F);
        mirrorPlane->setMaterial(mirrorMaterial);

        renderer->getScene().setGeometries({lightPlane, mirrorPlane});

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 45.0F, 0.0F));
        auto lightPlaneInstance = kuafu::instance(lightPlane, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.1F));
        transform = glm::rotate(transform, glm::radians(180.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        transform = glm::rotate(transform, glm::radians(90.0F), glm::vec3(1.0F, 0.0F, 0.0F));
        transform = glm::translate(transform, glm::vec3(0.0F, -100.0F, 0.0F));
        auto mirrorPlaneInstance1 = kuafu::instance(mirrorPlane, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.1F));
        transform = glm::rotate(transform, glm::radians(90.0F), glm::vec3(1.0F, 0.0F, 0.0F));
        transform = glm::translate(transform, glm::vec3(0.0F, -100.0F, 0.0F));
        auto mirrorPlaneInstance2 = kuafu::instance(mirrorPlane, transform);

        renderer->getScene().setGeometryInstances({mirrorPlaneInstance1, mirrorPlaneInstance2, lightPlaneInstance});

        for (int i = 1; i < 25000; ++i) {
            addSphere(renderer);
        }
    } else if (scene == Level::eSponza) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(100); // Will give a warning.
        renderer->getConfig().setGeometryInstanceLimit(1000);
        renderer->getConfig().setTextureLimit(50); // Will give a warning.
        renderer->getConfig().setAccumulatingFrames(true);
        renderer->getConfig().setClearColor(glm::vec4(1.0F));

        renderer->getScene().removeEnvironmentMap();
        renderer->getScene().getCamera()->setPosition(glm::vec3(-11.6F, 2.4F, -0.73F));
        renderer->getScene().getCamera()->setFront(glm::vec3(0.98F, 0.19F, 0.0F));

        auto lightPlane = kuafu::loadObj("models/plane.obj");
        kuafu::Material lightMaterial;
        lightMaterial.emission = glm::vec3(1.0F);
        lightPlane->setMaterial(lightMaterial);

        auto sponza = kuafu::loadObj("models/sponza/sponza.obj");

        renderer->getScene().setGeometries({lightPlane, sponza});

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 30.0F, 0.0F));
        auto lightPlaneInstance = kuafu::instance(lightPlane, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.01F));
        auto sponzaInstance = kuafu::instance(sponza, transform);

        renderer->getScene().setGeometryInstances({lightPlaneInstance, sponzaInstance});
    } else if (scene == Level::eNew) {
        renderer->reset();
        renderer->getConfig().setGeometryLimit(100); // Will give a warning.
        renderer->getConfig().setGeometryInstanceLimit(1000);
        renderer->getConfig().setTextureLimit(50); // Will give a warning.
        renderer->getConfig().setAccumulatingFrames(true);
        renderer->getConfig().setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));

        renderer->getScene().removeEnvironmentMap();
        renderer->getScene().getCamera()->setPosition(glm::vec3(-11.6F, 2.4F, -0.73F));
        renderer->getScene().getCamera()->setFront(glm::vec3(0.98F, 0.19F, 0.0F));

        auto floor = kuafu::loadObj("models/plane.obj");
        kuafu::Material floorMaterial;
//        floorMaterial.illum = 2;
        floorMaterial.emission = glm::vec3(1.0F);
        floorMaterial.kd = glm::vec3(1.0F);
        floor->setMaterial(floorMaterial);

        auto transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.2F, 0.2F, 0.2F));
        auto floorInstance = kuafu::instance(floor, transform);

        renderer->getScene().setGeometries({floor});
        renderer->getScene().setGeometryInstances({floorInstance});

//        renderer->getScene().removeEnvironmentMap();
//        renderer->getScene().getCamera()->setPosition(glm::vec3(-11.6F, 2.4F, -0.73F));
//        renderer->getScene().getCamera()->setFront(glm::vec3(0.98F, 0.19F, 0.0F));
//
//        auto lightPlane = kuafu::loadObj("models/plane.obj");
//        kuafu::Material lightMaterial;
//        lightMaterial.emission = glm::vec3(1.0F);
//        lightPlane->setMaterial(lightMaterial);
//
//        auto sponza = kuafu::loadObj("models/sponza/sponza.obj");
//
//        renderer->getScene().setGeometries({lightPlane, sponza});
//
//        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 30.0F, 0.0F));
//        auto lightPlaneInstance = kuafu::instance(lightPlane, transform);
//
//        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.01F));
//        auto sponzaInstance = kuafu::instance(sponza, transform);
//

    }
}

void updateScene(kuafu::Kuafu *renderer) {
    if (Key::eB) {
        for (int i = 0; i < 100; ++i) {
            addBox(renderer);
        }

        Key::eB = false;
    }

    if (Key::eL) {
        for (int i = 0; i < 100; ++i) {
            addSphere(renderer);
        }

        Key::eL = false;
    }

    if (currentLevel == Level::eAnimations) {
        auto instance = renderer->getScene().getGeometryInstance(0);

        if (instance != nullptr) {
            instance->setTransform(
                    glm::rotate(instance->transform, kuafu::Time::getDeltaTime() * 0.1F, glm::vec3(0.0F, 1.0F, 0.0F)));
        }
    } else if (currentLevel == Level::eMirrors) {
        auto instances = renderer->getScene( ).getGeometryInstances( );
        for ( auto instance : instances )
        {
          instance->setTransform( glm::rotate( instance->transform, kuafu::Time::getDeltaTime( ) * 0.1F, glm::vec3( 0.0F, 1.0F, 0.0F ) ) );
        }
    }
}
