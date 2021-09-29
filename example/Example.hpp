#include "kuafu.hpp"
#include <random>

enum class Level {
    eCornell,
    eAnimations,
    eSpheres,
    eMirrors,
    eSponza,
    eNew,
    eObj,
    eActive,
    eGLTF,
    eHide,
    eTexture,
    eFanbo,
    eGround
};

inline Level currentLevel;
auto dLight = std::make_shared<kuafu::DirectionalLight>();

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

inline void addScene(
        kuafu::Scene* scene,
        std::string_view path,
        glm::mat4 transform = glm::mat4(1.0F),
        kuafu::NiceMaterial* mat = nullptr) {
    auto model = scene->findGeometry(path);
    if (model == nullptr) {
        auto models = kuafu::loadScene(path, true);
        for (auto& m: models) {
            if (mat)
                m->setMaterial(*mat);
            scene->submitGeometry(m);
            scene->submitGeometryInstance(kuafu::instance(m, transform));
        }
    } else {
        scene->submitGeometryInstance(kuafu::instance(model, transform));
    }
}

inline void loadScene(kuafu::Scene *scene, Level level) {
    scene->getCamera()->resetView();

    if (level == Level::eCornell) {
        scene->setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 0.2F));
        scene->getCamera( )->setPosition( glm::vec3( 0.0F, 0.0F, -0.6F ) );
        scene->getCamera( )->setFront( glm::vec3( 0.0F, 0.0F, -1.F ) );
        scene->getCamera( )->setUp( glm::vec3( 0.0F, 1.0F, 0.F ) );

        auto cornell = kuafu::loadScene("models/CornellBox.obj", false);

        for (auto& c: cornell)
            scene->submitGeometry(c);

        for (auto& c: cornell) {
            auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, -1.0F, -1.0F));
            auto cornellInstance = kuafu::instance(c, transform);
            scene->submitGeometryInstance(cornellInstance);
        }

        scene->removeEnvironmentMap();
    } else if (level == Level::eAnimations) {
        scene->setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));

        // Load geometries.
        auto plane = kuafu::loadObj("models/plane.obj");

        // Make a custom material for an emissive surface (light source).
        kuafu::NiceMaterial customMaterial;
        customMaterial.emission = glm::vec3(1.0F);
        plane->setMaterial(customMaterial);

        // Submit geometries.
        scene->setGeometries({plane});

        // Create instances of the geometries.
        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 80.0F, 0.0F));
        auto planeInstance = kuafu::instance(plane, transform);

        // Submit instances for drawing.
        scene->setGeometryInstances({planeInstance});

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.25F));
        transform = glm::rotate(transform, glm::radians(45.0F), glm::vec3(0.0F, 1.0F, 0.0F));
        transform = glm::translate(transform, glm::vec3(0.0F, -2.0F, 1.0F));

        addScene(scene, "models/scene.obj", transform);

        scene->removeEnvironmentMap();
    } else if (level == Level::eSpheres) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.3));
//        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.0));

        scene->getCamera()->setPosition(glm::vec3(-12.6F, 0.0F, 15.4F));
        scene->getCamera()->setFront(glm::vec3(0.67F, 0.0F, -0.8F));

//        auto dLight = std::make_shared<kuafu::DirectionalLight>();
        dLight->direction ={-2, -1, -1};
//        dLight->direction ={1., 0.1, -0.5};
        dLight->color = {1., 0.9, 0.7};
        dLight->strength = 8;
        dLight->softness = 0.5;
        scene->setDirectionalLight(dLight);

//        auto pLight = std::make_shared<kuafu::PointLight>();
//        pLight->position ={0, 0, 4};
//        pLight->color = {1., 0.5, 0.5};
//        pLight->strength = 100;
//        pLight->radius = 0.5;
//        scene->addPointLight(pLight);

//        pLight = std::make_shared<kuafu::PointLight>();
//        pLight->position ={-6, 0, 3};
//        pLight->color = {0.5, 0.5, 1.0};
//        pLight->strength = 100;
//        pLight->radius = 0.5;
//        scene->addPointLight(pLight);
//
//        pLight = std::make_shared<kuafu::PointLight>();
//        pLight->position ={2, 5, 5};
//        pLight->color = {0.5, 1.0, 0.0};
//        pLight->strength = 100;
//        pLight->radius = 0.5;
//        scene->addPointLight(pLight);

//        auto aLight = std::make_shared<kuafu::ActiveLight>();
////        aLight->position ={0, 0, 7};
////        aLight->direction ={0, 0, -1};
//        aLight->fov = glm::radians(150.F);
//        auto pos = glm::lookAt(glm::vec3{-3., -3., 8.}, {0., 0., 0.}, {-1., 0.5, 0});
//        aLight->viewMat = pos;
//
//        aLight->color = {1., 1., 1.};
//        aLight->strength = 1000;
//        aLight->softness = 1;
//        aLight->texPath = "../resources/d415-pattern-sq.png";
////        aLight->texPath = "../resources/py2.png";
////        aLight->texPath = "";
//        scene->addActiveLight(aLight);

        kuafu::NiceMaterial floorMaterial;
//        floorMaterial.emission = glm::vec3(1.0, 0.2, 0.2);
        floorMaterial.diffuseColor = glm::vec3(0.8);
        floorMaterial.specular = 0.5;
        floorMaterial.metallic = 0;
//        floorMaterial.metallic = 0.9;
        floorMaterial.transmission = 0;
        floorMaterial.roughness = 0.1;
        auto floor = kuafu::createYZPlane(true, floorMaterial);

        kuafu::NiceMaterial mat;
        mat.diffuseColor = glm::vec3(1.0F, 0.7F, 0.7F);
//        mat.diffuseColor = glm::vec3(0.275,0.059,0.0);
//        mat.diffuseColor = glm::vec3(0.1F);
        mat.specular = 0.0F;
        mat.metallic = 0.1F;
        mat.roughness = 0.01F;
        mat.ior = 1.45F;
//        mat.ior = 5.0F;
        mat.alpha = 1.0F;
        mat.transmission = 1.0F;
//        auto sphere0 = kuafu::createSphere(true, mat);
        auto sphere0 = kuafu::createCube(true, mat);

        mat.diffuseColor = glm::vec3(0.7F, 0.40F, 0.1F);
        mat.specular = 0.0F;
        mat.metallic = 1.0F;
//        mat.metallic = 0.0F;
        mat.roughness = 0.07F;
        mat.ior = 1.4F;
        mat.alpha = 1.0F;
        mat.transmission = 0.0F;
//        mat->transmission = 1.0F;
        auto sphere = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.2F;
        mat.transmission = 0.0F;
        auto sphere1 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.4F;
        auto sphere2 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.6F;
        auto sphere3 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.8F;
        auto sphere4 = kuafu::createSphere(true, mat);

        mat.diffuseColor = glm::vec3(0.9);
        mat.specular = 1.0F;
        mat.roughness = 0.0F;
        mat.metallic = 1.0F;
        auto glass = kuafu::createYZPlane(true, mat);

        mat.diffuseColor = glm::vec3(1);
        mat.specular = 0.0F;
        mat.metallic = 0.0F;
        mat.roughness = 0.0F;
        mat.ior = 1.4F;
        mat.alpha = 1.0F;
        mat.transmission = 1.0F;
//        auto cube = kuafu::createCube(true, mat);
        auto cube = kuafu::createCapsule(2, 1, true, mat);



        scene->setGeometries(
                {
                    floor,
                    sphere0,
                    sphere,
                 sphere1, sphere2, sphere3, sphere4, glass,
//                 obj,
                 cube
                });

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, -1.F));
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        transform = glm::scale(transform, glm::vec3(12.0F, 12.0F, 12.0F));
        auto floorInstance = kuafu::instance(floor, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-1.F, -2.0F, 4.F));
        transform = glm::scale(transform, glm::vec3(2));
        auto sphereInstance0 = kuafu::instance(sphere0, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-5.0F, 0.0F, 0.0F));
//        transform = glm::scale(transform, glm::vec3(0.5));
        auto sphereInstance = kuafu::instance(sphere, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-2.5F, 0.0F, 0.0F));
        auto sphereInstance1 = kuafu::instance(sphere1, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
        auto sphereInstance2 = kuafu::instance(sphere2, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(2.5F, 0.0F, 0.0F));
        auto sphereInstance3 = kuafu::instance(sphere3, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(5.0F, 0.0F, 0.0F));
        auto sphereInstance4 = kuafu::instance(sphere4, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(7.0F, 0.0F, 0.0F));
        transform = glm::rotate(transform, glm::radians(180.F), {0., 1., 0.});
        transform = glm::scale(transform, glm::vec3(8.F));
        auto glassInstance = kuafu::instance(glass, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-4.5F, -4.0F, 0.5F));
        transform = glm::rotate(transform, glm::radians(-45.F), {0., 0., 1.});
        transform = glm::rotate(transform, glm::radians(45.F), {1., 1., 0.});
        transform = glm::scale(transform, glm::vec3(1));
        auto cubeInstance = kuafu::instance(cube, transform);


        scene->setGeometryInstances(
                {
                    floorInstance,
                    sphereInstance0,
                    sphereInstance,
                 sphereInstance1, sphereInstance2, sphereInstance3,
                 sphereInstance4, glassInstance,
//                 objInstance,
                 cubeInstance
                });


        mat.diffuseColor = glm::vec3(0.2F, 0.2F, 0.2F);
        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.05F;
        mat.ior = 1.45F;
//        mat.ior = 5.0F;
        mat.alpha = 1.0F;
        mat.transmission = 0.0F;

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-2.0F, 5.0F, 2.F));
        transform = glm::rotate(transform, glm::radians(-90.f), glm::vec3{0, 0, 1});
        transform = glm::rotate(transform, glm::radians(-20.f), glm::vec3{1, 0, 0});
        transform = glm::scale(transform, glm::vec3(3));

        addScene(scene, "../resources/models/suzanne.dae", transform, &mat);


    } else if (level == Level::eMirrors) {
        scene->setClearColor(glm::vec4(0.5F, 0.5F, 0.7F, 1.0F));

        scene->getCamera()->setPosition(glm::vec3(9.8F, 0.3F, 7.7F));
        scene->getCamera()->setFront(glm::vec3(-0.5F, 0.0F, -0.8F));
        //scene->getCamera( )->setFront( glm::vec3( -0.5F, 0.0F, -0.8F ) );
        scene->removeEnvironmentMap();

        auto lightPlane = kuafu::loadObj("models/plane.obj");
        kuafu::NiceMaterial lightMaterial;
        lightMaterial.emission = glm::vec3(1.0F);
        lightPlane->setMaterial(lightMaterial);

        auto mirrorPlane = kuafu::loadObj("models/plane.obj");
        kuafu::NiceMaterial mirrorMaterial;
        mirrorMaterial.roughness = 0.0F;
        mirrorMaterial.diffuseColor = glm::vec3(0.95F);
        mirrorPlane->setMaterial(mirrorMaterial);

        scene->setGeometries({lightPlane, mirrorPlane});

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

        scene->setGeometryInstances({mirrorPlaneInstance1, mirrorPlaneInstance2, lightPlaneInstance});

    } else if (level == Level::eSponza) {
        scene->setClearColor(glm::vec4(1.0F));

        scene->removeEnvironmentMap();
        scene->getCamera()->setPosition(glm::vec3(-11.6F, 2.4F, -0.73F));
        scene->getCamera()->setFront(glm::vec3(0.98F, 0.19F, 0.0F));

        auto lightPlane = kuafu::loadObj("models/plane.obj");
        kuafu::NiceMaterial lightMaterial;
        lightMaterial.emission = glm::vec3(1.0F);
        lightPlane->setMaterial(lightMaterial);

        auto sponza = kuafu::loadObj("models/sponza/sponza.obj");

        scene->setGeometries({lightPlane, sponza});

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 30.0F, 0.0F));
        auto lightPlaneInstance = kuafu::instance(lightPlane, transform);

        transform = glm::scale(glm::mat4(1.0F), glm::vec3(0.01F));
        auto sponzaInstance = kuafu::instance(sponza, transform);

        scene->setGeometryInstances({lightPlaneInstance, sponzaInstance});
    } else if (level == Level::eNew) {
//        scene->setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.1F));

        scene->getCamera()->setPosition(glm::vec3(-6.F, 0.F, 0.2F));
        scene->getCamera()->setFront(glm::vec3(1.F, 0.0F, 0.F));

        auto dLight = std::make_shared<kuafu::DirectionalLight>();
        dLight->direction ={1, 1, -1};
//        dLight->direction ={1., 0.1, -0.5};
        dLight->color = {1., 1., 1.};
        dLight->strength = 20;
        dLight->softness = 0.1;
        scene->setDirectionalLight(dLight);

        kuafu::NiceMaterial floorMaterial;
        floorMaterial.diffuseColor = glm::vec3(1, 1, 1);
        auto floor = kuafu::createYZPlane(true, floorMaterial);

        kuafu::NiceMaterial mat;;
        mat.diffuseColor = glm::vec3(0.7F, 0.40F, 0.1F);
        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.2F;
        mat.ior = 1.4F;
        mat.alpha = 10.F;
        auto cap0 = kuafu::createCapsule(0.08, 0.05, true, mat);
        auto cap1 = kuafu::createCapsule(0.1, 0.1, true, mat);
        auto cap2 = kuafu::createCapsule(0.2, 0.2, true, mat);
        auto cap3 = kuafu::createCapsule(0.3, 0.3, true, mat);
        auto cap4 = kuafu::createCapsule(0.4, 0.4, true, mat);

        scene->setGeometries({floor, cap0, cap1, cap2, cap3, cap4});

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
        transform = glm::scale(transform, glm::vec3(5.0F, 5.0F, 5.0F));
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        auto floorInstance = kuafu::instance(floor, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, -2.0F, 0.05F));
        auto cap0Instance = kuafu::instance(cap0, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, -1.0F, 0.1F));
        auto cap1Instance = kuafu::instance(cap1, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.2F));
        auto cap2Instance = kuafu::instance(cap2, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 1.0F, 0.3F));
        auto cap3Instance = kuafu::instance(cap3, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 2.0F, 0.4F));
        auto cap4Instance = kuafu::instance(cap4, transform);

        scene->setGeometryInstances({
            floorInstance, cap0Instance, cap1Instance, cap2Instance, cap3Instance, cap4Instance});
    } else if (level == Level::eObj) {
//        scene->setClearColor(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 1.0F));

        scene->getCamera()->setPosition(glm::vec3(-6.F, 0.F, 0.2F));
        scene->getCamera()->setFront(glm::vec3(1.F, 0.0F, 0.F));

        kuafu::NiceMaterial floorMaterial;
        floorMaterial.diffuseColor = glm::vec3(1.0, 0.3, 0.3);
        auto floor = kuafu::createYZPlane(true, floorMaterial);

        kuafu::NiceMaterial mat;
        mat.diffuseColor = glm::vec3(0.7F, 0.40F, 0.1F);
        mat.specular = 100.0F;
        mat.roughness = 10.;
        mat.ior = 10.0F;
        mat.alpha = 1.0;

        auto objs = kuafu::loadScene(
                "/zdata/ssource/ICCV2021_Diagnosis/description/xarm_description/meshes/optical_table/visual/optical_table.dae", true);

        scene->setGeometries({floor});
        for (auto& g: objs)
            scene->submitGeometry(g);

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
        transform = glm::scale(transform, glm::vec3(5.0F, 5.0F, 5.0F));
        transform = glm::rotate(transform, glm::radians(90.F), {0., 1., 0.});
        auto floorInstance = kuafu::instance(floor, transform);

        scene->setGeometryInstances({floorInstance});
        for (auto& g: objs)
            scene->submitGeometryInstance(instance(g, glm::mat4(1.0F)));

    } else if (level == Level::eActive) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.0));

        scene->getCamera()->setPosition(glm::vec3(-12.6F, 1.1F, 15.4F));
        scene->getCamera()->setFront(glm::vec3(0.67F, 0.0F, -0.8F));


        auto aLight = std::make_shared<kuafu::ActiveLight>();
//        aLight->position ={0, 0, 7};
//        aLight->direction ={0, 0, -1};
        aLight->fov = glm::radians(150.F);
        auto pos = glm::lookAt(glm::vec3{-3., -3., 8.}, {0., 0., 0.}, {-1., 0.5, 0});
        aLight->viewMat = pos;

        aLight->color = {1., 1., 1.};
        aLight->strength = 1000;
        aLight->softness = 1;
        aLight->texPath = "../resources/d415-pattern-sq.png";
//        aLight->texPath = "../resources/py2.png";
//        aLight->texPath = "";
        scene->addActiveLight(aLight);

        kuafu::NiceMaterial floorMaterial;
//        floorMaterial->emission = glm::vec3(1.0, 0.2, 0.2);
        floorMaterial.diffuseColor = glm::vec3(0.8);
        floorMaterial.specular = 0.5;
//        floorMaterial.metallic = 0;
        floorMaterial.metallic = 0.9;
        floorMaterial.transmission = 0;
        floorMaterial.roughness = 0.1;
        auto floor = kuafu::createYZPlane(true, floorMaterial);

        kuafu::NiceMaterial mat;
        mat.diffuseColor = glm::vec3(1.0F, 0.7F, 0.7F);
        mat.specular = 0.0F;
        mat.metallic = 0.0F;
        mat.roughness = 0.0F;
        mat.ior = 1.45F;
//        mat.ior = 5.0F;
        mat.alpha = 1.0F;
        mat.transmission = 1.0F;
//        auto sphere0 = kuafu::createSphere(true, mat);
//        auto sphere0 = kuafu::createCube(true, mat);

        mat.diffuseColor = glm::vec3(0.7F, 0.40F, 0.1F);
        mat.specular = 0.0F;
        mat.metallic = 1.0F;
//        mat.metallic = 0.0F;
        mat.roughness = 0.07F;
        mat.ior = 1.4F;
        mat.alpha = 1.0F;
        mat.transmission = 0.0F;
//        mat.transmission = 1.0F;
        auto sphere = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.2F;
        mat.transmission = 0.0F;
        auto sphere1 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.4F;
        auto sphere2 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.6F;
        auto sphere3 = kuafu::createSphere(true, mat);

        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.8F;
        auto sphere4 = kuafu::createSphere(true, mat);

        mat.diffuseColor = glm::vec3(0.9);
        mat.specular = 1.0F;
        mat.roughness = 0.0F;
        mat.metallic = 1.0F;
        auto glass = kuafu::createYZPlane(true, mat);

        mat.diffuseColor = glm::vec3(1);
        mat.specular = 0.0F;
        mat.metallic = 0.0F;
        mat.roughness = 0.0F;
        mat.ior = 1.4F;
        mat.alpha = 1.0F;
        mat.transmission = 1.0F;
//        auto cube = kuafu::createCube(true, mat);
        auto cube = kuafu::createCapsule(2, 1, true, mat);

        scene->setGeometries(
                {
                        floor,
//                    sphere0,
                        sphere,
                        sphere1, sphere2, sphere3, sphere4, glass,
//                 obj,
                        cube
                });

        auto transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, -1.F));
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        transform = glm::scale(transform, glm::vec3(12.0F, 12.0F, 12.0F));
        auto floorInstance = kuafu::instance(floor, transform);

//        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-1.F, -2.0F, 4.F));
//        transform = glm::scale(transform, glm::vec3(2));
//        auto sphereInstance0 = kuafu::instance(sphere0, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-5.0F, 0.0F, 0.0F));
//        transform = glm::scale(transform, glm::vec3(0.5));
        auto sphereInstance = kuafu::instance(sphere, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-2.5F, 0.0F, 0.0F));
        auto sphereInstance1 = kuafu::instance(sphere1, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F));
        auto sphereInstance2 = kuafu::instance(sphere2, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(2.5F, 0.0F, 0.0F));
        auto sphereInstance3 = kuafu::instance(sphere3, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(5.0F, 0.0F, 0.0F));
        auto sphereInstance4 = kuafu::instance(sphere4, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(7.0F, 0.0F, 0.0F));
        transform = glm::rotate(transform, glm::radians(180.F), {0., 1., 0.});
        transform = glm::scale(transform, glm::vec3(8.F));
        auto glassInstance = kuafu::instance(glass, transform);

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-4.5F, -4.0F, 0.5F));
        transform = glm::rotate(transform, glm::radians(-45.F), {0., 0., 1.});
        transform = glm::rotate(transform, glm::radians(45.F), {1., 1., 0.});
        transform = glm::scale(transform, glm::vec3(1));
        auto cubeInstance = kuafu::instance(cube, transform);


        scene->setGeometryInstances(
                {
                        floorInstance,
//                    sphereInstance0,
                        sphereInstance,
                        sphereInstance1, sphereInstance2, sphereInstance3,
                        sphereInstance4, glassInstance,
//                 objInstance,
                        cubeInstance
                });


        mat.diffuseColor = glm::vec3(0.2F, 0.2F, 0.2F);
        mat.specular = 0.0F;
        mat.metallic = 1.0F;
        mat.roughness = 0.05F;
        mat.ior = 1.45F;
//        mat.ior = 5.0F;
        mat.alpha = 1.0F;
        mat.transmission = 0.0F;

        transform = glm::translate(glm::mat4(1.0F), glm::vec3(-2.0F, 5.0F, 2.F));
        transform = glm::rotate(transform, glm::radians(-90.f), glm::vec3{0, 0, 1});
        transform = glm::rotate(transform, glm::radians(-20.f), glm::vec3{1, 0, 0});
        transform = glm::scale(transform, glm::vec3(3));

        addScene(scene, "../resources/models/suzanne.dae", transform, &mat);


    } else if (level == Level::eGLTF) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 1.0));

        scene->getCamera()->setPosition(glm::vec3(0.5F, 0.5F, 0.5F));
        scene->getCamera()->setFront(glm::vec3(-1.F, -1.F, -1.F));

        addScene(scene, "/home/jet/Downloads/toc.glb");
//        addScene(scene, "/home/jet/Downloads/test.glb");
//        addScene(scene, "/home/jet/Downloads/model.blend");

    } else if (level == Level::eHide) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.5));

        scene->getCamera()->setPosition(glm::vec3(0.3F, 0.3F, 0.3F));
        scene->getCamera()->setFront(glm::vec3(-1.F, -1.F, -1.F));

        auto dLight = std::make_shared<kuafu::DirectionalLight>();
        dLight->direction ={-1, 1, -1};
        dLight->color = {1., 1., 1.};
        dLight->strength = 3;
        dLight->softness = 0.1;
        scene->setDirectionalLight(dLight);

        auto floor = kuafu::createYZPlane(false);
        auto transform = glm::translate(glm::mat4(1.0F), {0, 0, -0.1});
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        transform = glm::scale(transform, glm::vec3(1.0F, 1.0F, 1.0F));
        auto floorInstance = kuafu::instance(floor, transform);

        scene->setGeometries({floor});
        scene->setGeometryInstances({floorInstance});
        addScene(scene,
                 "/zdata/ssource/ICCV2021_Diagnosis/ocrtoc_materials/models/tennis_ball/visual_mesh.obj");
    } else if (level == Level::eTexture) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.5));

        scene->getCamera()->setPosition(glm::vec3(0.3F, 0.3F, 0.3F));
        scene->getCamera()->setFront(glm::vec3(-1.F, -1.F, -1.F));

        auto dLight = std::make_shared<kuafu::DirectionalLight>();
        dLight->direction ={-1, 1, -1};
        dLight->color = {1., 1., 1.};
        dLight->strength = 3;
        dLight->softness = 0.1;
        scene->setDirectionalLight(dLight);

        auto floor = kuafu::createYZPlane(false);
        auto transform = glm::translate(glm::mat4(1.0F), {0, 0, -0.1});
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        transform = glm::scale(transform, glm::vec3(1.0F, 1.0F, 1.0F));
        auto floorInstance = kuafu::instance(floor, transform);

        scene->setGeometries({floor});
        scene->setGeometryInstances({floorInstance});

        kuafu::NiceMaterial mat {
            .diffuseTexPath = "/home/jet/Downloads/textures/diffuse.jpg",
            .metallicTexPath = "/home/jet/Downloads/textures/metallic.jpg",
            .transmissionTexPath = "/home/jet/Downloads/textures/transmission.jpg",
            .metallic = 0.0,
            .specular = 0.4,
            .roughness = 0.4,
            .ior = 1.33,
            .transmission = 1.0
        };

        addScene(scene,
                 "/zdata/ssource/ICCV2021_Diagnosis/ocrtoc_materials/models/spellegrino/visual_mesh.obj",
                 glm::mat4(1.0), &mat);

//        addScene(scene,
//                 "/zdata/ssource/ICCV2021_Diagnosis/ocrtoc_materials/models/rubik/visual_mesh.obj",
//                 glm::mat4(1.0));
    } else if (level == Level::eFanbo) {
        for (int i = 0; i < 2000; i++) {
            auto sp = kuafu::createSphere();
            scene->submitGeometry(sp);
            scene->submitGeometryInstance(instance(sp, glm::mat4(1.0F)));
        }
    } else if (level == Level::eGround) {
        scene->setClearColor(glm::vec4(0.64F, 0.60F, 0.52F, 0.5));

        scene->getCamera()->setPosition(glm::vec3(0.3F, 0.3F, 0.3F));
        scene->getCamera()->setFront(glm::vec3(-1.F, -1.F, -1.F));

        auto floor = kuafu::createYZPlane(false);
        auto transform = glm::translate(glm::mat4(1.0F), {0, 0, -0.1});
        transform = glm::rotate(transform, glm::radians(90.F), {0., -1., 0.});
        transform = glm::scale(transform, glm::vec3(1.0F, 1.0F, 1.0F));
        auto floorInstance = kuafu::instance(floor, transform);

        auto dLight = std::make_shared<kuafu::DirectionalLight>();
        dLight->direction ={-1, 1, -1};
        dLight->color = {1., 1., 1.};
        dLight->strength = 3;
        dLight->softness = 0.1;
        scene->setDirectionalLight(dLight);

        scene->setGeometries({floor});
        scene->setGeometryInstances({floorInstance});
    }
}

void updateScene(kuafu::Scene* scene) {
//    if (currentLevel == Level::eAnimations) {
//        auto instance = renderer.getScene()->getGeometryInstance(0);
//
//        if (instance != nullptr) {
//            instance->setTransform(
//                    glm::rotate(instance->transform, kuafu::Time::getDeltaTime() * 0.1F, glm::vec3(0.0F, 1.0F, 0.0F)));
//        }
//    } else if (currentLevel == Level::eMirrors) {
//        auto instances = renderer.getScene( ).getGeometryInstances( );
//        for ( auto instance : instances )
//        {
//          instance->setTransform( glm::rotate( instance->transform, kuafu::Time::getDeltaTime( ) * 0.1F, glm::vec3( 0.0F, 1.0F, 0.0F ) ) );
//        }
//    }

//    if (currentLevel == Level::eSpheres) {
//        dLight->direction += glm::vec3{0.01, 0, 0};
//        kuafu::global::frameCount = -1;
//    }
//    static int cnt = 0;
//    cnt++;
//
//    if (cnt % 10 == 5) {
//        auto geometry = scene->getGeometryByGlobalIndex(1);
//        geometry->hideRender = true;
//        scene->markGeometriesChanged();
//        scene->markGeometryInstancesChanged();
//        kuafu::global::frameCount = -1;
//    }
//    if (cnt % 10 == 0) {
//        auto geometry = scene->getGeometryByGlobalIndex(1);
//        geometry->hideRender = false;
//        scene->markGeometriesChanged();
//        scene->markGeometryInstancesChanged();
//        kuafu::global::frameCount = -1;
//    }
}
