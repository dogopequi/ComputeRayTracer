#include "Renderer.h"

Renderer::Renderer(bool fullscreen, int windowWidth, int windowHeight, int imageWidth, int imageHeight, std::string name)
    : SCR_WIDTH(windowWidth), SCR_HEIGHT(windowHeight), IMAGE_WIDTH(imageWidth), IMAGE_HEIGHT(imageHeight), m_Name(name)
{
    window = new Window(SCR_WIDTH, SCR_HEIGHT, m_Name, fullscreen);
    if (fullscreen)
    {
        Fullscreen = fullscreen;
    }
    else
    {
        Interactive = fullscreen;
    }

    indexLabelMap.insert({ 1.0f, "Metal" });
    indexLabelMap.insert({ 2.0f, "Lambertian" });
    indexLabelMap.insert({ 3.0f, "Dielectric" });
    indexLabelMap.insert({ 4.0f, "Emissive" });
}

Renderer::~Renderer()
{
    SaveScenes();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &normalImage);
    glDeleteTextures(1, &accumulatedImage);
    glDeleteProgram(compute->m_ShaderID);
    if (Fullscreen)
    {
        glDeleteProgram(pipeline->m_ShaderID);
    }
    glDeleteProgram(accumulation->m_ShaderID);

    delete compute;
    delete accumulation;
    delete pipeline;
    delete window;

    glfwTerminate();
}

void Renderer::Render()
{
    int width, height;
    if (Fullscreen)
    {
        width = SCR_WIDTH;
        height = SCR_HEIGHT;
    }
    else
    {
        width = IMAGE_WIDTH;
        height = IMAGE_HEIGHT;
    }

    preRender(width, height);

    Scene currentScene{};

    while (!glfwWindowShouldClose(window->getWindow()))
    {
        currentScene = m_Scenes[selectedList];
        auto start_time = std::chrono::steady_clock::now();
        controls();
        glfwPollEvents();
        SphereBuffer1.clear();
        SphereBuffer2.clear();
        SphereBuffer3.clear();
        SphereBuffer4.clear();
        for (const auto& sphere : currentScene.spheres)
        {
            SphereBuffer1.push_back(sphere.center.x);
            SphereBuffer1.push_back(sphere.center.y);
            SphereBuffer1.push_back(sphere.center.z);
            SphereBuffer1.push_back(sphere.radius);
            SphereBuffer2.push_back(sphere.color.x);
            SphereBuffer2.push_back(sphere.color.y);
            SphereBuffer2.push_back(sphere.color.z);
            SphereBuffer2.push_back(sphere.index);
            SphereBuffer3.push_back(sphere.fuzziness);
            SphereBuffer3.push_back(sphere.refraction);
            SphereBuffer3.push_back(sphere.emissionPower);
            SphereBuffer3.push_back(sphere.fuzziness);//filler
            SphereBuffer4.push_back(sphere.sPosition.x);
            SphereBuffer4.push_back(sphere.sPosition.y);
            SphereBuffer4.push_back(sphere.sPosition.z);
            if (sphere.isMoving)
            {
                SphereBuffer4.push_back(1.0f);
            }
            else
            {
                SphereBuffer4.push_back(0.0f);
            }
        }

        glUseProgram(compute->m_ShaderID);

        compute->enable();
        compute->setUniform1i("sphereCount", currentScene.spheres.size());
        compute->setUniform1f("FOV", currentScene.fov);
        compute->setUniform3f("lookFrom", currentScene.lookFrom);
        compute->setUniform3f("lookAt", currentScene.lookAt);
        compute->setUniform3f("vUp", currentScene.vUp);
        compute->setUniform1f("defocusAngle", currentScene.defocusAngle);
        compute->setUniform1f("focusDist", currentScene.focusDist);
        compute->setUniform1i("shouldBlur", currentScene.blur);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, SphereBuffer1.size() * sizeof(float), SphereBuffer1.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo1);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, SphereBuffer2.size() * sizeof(float), SphereBuffer2.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo2);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo3);
        glBufferData(GL_SHADER_STORAGE_BUFFER, SphereBuffer3.size() * sizeof(float), SphereBuffer3.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo3);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo4);
        glBufferData(GL_SHADER_STORAGE_BUFFER, SphereBuffer4.size() * sizeof(float), SphereBuffer4.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo4);

        compute->setUniform1f("aspectRatio", aspectRatio);
        compute->setUniform1i("height", height);
        compute->setUniform1i("width", width);
        compute->setUniform1f("random", randomFloat());
        compute->setUniform1i("bounces", bounces);
        compute->setUniform1i("samples", samples);
        compute->setUniform1f("time", deltaTime);
        compute->setUniform1f("frame", frame);
        compute->setUniform3f("BackGroundColor", currentScene.BackgroundColor);

        glDispatchCompute(width / worksizeGroup.x, height / worksizeGroup.y, 1);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glUseProgram(accumulation->m_ShaderID);

        accumulation->setUniform1i("frame", frame);
        accumulation->setUniform2ui("imageSize", glm::vec2(width, height));

        if (accumulate)
        {
            accumulation->setUniform1i("accumulate", 1);
        }
        else
        {
            accumulation->setUniform1i("accumulate", 0);
        }
        
        glDispatchCompute(width / worksizeGroup.x, height / worksizeGroup.y, 1);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glBindTexture(GL_TEXTURE_2D, accumulatedImage);
        if (writeImage)
        {
            std::string folder = "Resources/Images/";
            std::string f = std::string(str0);
            std::string g = std::string(".png");
            if (!std::filesystem::exists(folder + f + g))
            {
                GLubyte* data;
                data = (GLubyte*)malloc(width * height * 4 * sizeof(GLubyte));

                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                filexists = false;
                pipeline->writeImage(folder + f + g, width, height, 4, data, width * 4);
            }
            else
            {
                filexists = true;
            }
        }

        writeImage = false;

        if(startRendering)
        {
            glUseProgram(pipeline->m_ShaderID);

            pipeline->enable();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, accumulatedImage);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        ImGuiRender();

        glfwSwapInterval(1);
        glfwSwapBuffers(window->getWindow());

        frame++;


        auto end_time = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        if (deltaTime > 0)
            frames = CLOCKS_PER_SEC / deltaTime;
    }
}

void Renderer::Init(std::string fragment, std::string vertex, std::string computeBase, std::string computeAccumulate)
{

    LoadScenes();
    glGenBuffers(1, &ssbo1);
    glGenBuffers(1, &ssbo2);
    glGenBuffers(1, &ssbo3);
    glGenBuffers(1, &ssbo4);
    if (Fullscreen)
    {
        pipeline = new Shader(vertex.c_str(), fragment.c_str());
        compute = new Shader(computeBase.c_str());
        accumulation = new Shader(computeAccumulate.c_str());
    }
    else
    {
        compute = new Shader(computeBase.c_str());
        accumulation = new Shader(computeAccumulate.c_str());
    }

    Render();
}

bool Renderer::LoadScenes()
{

    std::ifstream file(filePath);
    std::string line;
    std::stringstream stream;
    int scenes = -1;
    int spheres = -1;
    if (file.is_open())
    {
        for (std::string line; std::getline(file, line); )   //read stream line by line
        {
            if (line.find("#scene") != std::string::npos)
            {
                m_Scenes.emplace_back(Scene());
                scenes++;
                spheres = -1;
                continue;
            }
            if (line.find("#sphere") != std::string::npos)
            {
                m_Scenes[scenes].spheres.emplace_back(Sphere());
                spheres++;
                continue;
            }
            if (line.find("BackgroundCol") != std::string::npos)
            {
                m_Scenes[scenes].BackgroundColor = load3f(line, "BackgroundCol");
                continue;
            }
            if (line.find("pos") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].center = load3f(line, "pos");
                continue;
            }
            if (line.find("radius") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].radius = load1f(line, "radius");
                continue;
            }
            if (line.find("color") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].color = load3f(line, "color");
                continue;
            }
            if (line.find("sPosition") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].sPosition = load3f(line, "sPosition");
                continue;
            }
            if (line.find("Epower") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].emissionPower = load1f(line, "Epower");
                continue;
            }
            if (line.find("isMoving") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].isMoving = load1b(line, "isMoving");
                continue;
            }
            if (line.find("index") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].index = load1f(line, "index");
                continue;
            }
            if (line.find("fuzz") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].fuzziness = load1f(line, "fuzz");
                continue;
            }
            if (line.find("ref") != std::string::npos)
            {
                m_Scenes[scenes].spheres[spheres].refraction = load1f(line, "ref");
                continue;
            }
            if (line.find("fov") != std::string::npos)
            {
                m_Scenes[scenes].fov = load1f(line, "fov");
                continue;
            }
            if (line.find("blur") != std::string::npos)
            {
                m_Scenes[scenes].blur = load1f(line, "blur");
                continue;
            }
            if (line.find("defocusAngle") != std::string::npos)
            {
                m_Scenes[scenes].defocusAngle = load1f(line, "defocusAngle");
                continue;
            }
            if (line.find("focusDist") != std::string::npos)
            {
                m_Scenes[scenes].focusDist = load1f(line, "focusDist");
                continue;
            }
            if (line.find("lookFrom") != std::string::npos)
            {
                m_Scenes[scenes].lookFrom = load3f(line, "lookFrom");
                continue;
            }
            if (line.find("lookAt") != std::string::npos)
            {
                m_Scenes[scenes].lookAt = load3f(line, "lookAt");
                continue;
            }
            if (line.find("vUp") != std::string::npos)
            {
                m_Scenes[scenes].vUp = load3f(line, "vUp");
                continue;
            }
        }
        file.close();
    }
    else 
    {
        std::cerr << "Unable to open file" << std::endl;
        return 1;
    }

    return false;
}

void Renderer::ImGuiRender()
{
    ImGuiStartFrame();
    ImGuiSettings();
    ImGuiScene();
    ImGuiEndFrame();
}

void Renderer::OnResize(int width, int height)
{
    
    //TODO
}

void Renderer::controls()
{
    //not doing controls at the moment
    //if (window->isKeyPressed(GLFW_KEY_A))
    //{
    //    cameraCenter += glm::vec3(-0.01f, 0.0f, 0.0f);
    //}
    //if (window->isKeyPressed(GLFW_KEY_D))
    //{
    //    cameraCenter += glm::vec3(0.01f, 0.0f, 0.0f);
    //}
    //if (window->isKeyPressed(GLFW_KEY_W))
    //{
    //    cameraCenter += glm::vec3(0.0f, 0.01f, 0.0f);
    //}
    //if (window->isKeyPressed(GLFW_KEY_S))
    //{
    //    cameraCenter += glm::vec3(0.0f, -0.01f, 0.0f);
    //}

    //if (window->isKeyPressed(GLFW_KEY_UP))
    //{
    //    focalLength += 0.001f;
    //}
    //if (window->isKeyPressed(GLFW_KEY_DOWN))
    //{
    //    focalLength -= 0.001f;
    //}
}

float Renderer::load1f(std::string& line, std::string variable)
{
    std::stringstream in(line);
    std::string type;
    in >> type;

    if (type == variable)
    {
        float x;
        in >> x;
        return x;
    }
}

glm::vec3 Renderer::load3f(std::string& line, std::string variable)
{
    std::stringstream in(line);
    std::string type;
    in >> type;

    if (type == variable)
    {
        float x, y, z;
        in >> x >> y >> z;
        return glm::vec3(x, y, z);
    }
}

bool Renderer::load1b(std::string& line, std::string variable)
{
    std::stringstream in(line);
    std::string type;
    in >> type;

    if (type == variable)
    {
        bool x;
        in >> x;
        return x;
    }
}

bool Renderer::findFiles(std::string& name, std::string& type)
{

    std::string filename = "image.png";
    std::string folder = "Resources/Images/";
    std::ifstream file(folder);
    std::string line;
    std::stringstream stream;

    if (file.is_open())
    {
        for (std::string line; std::getline(file, line); )  
        {
            if (line.find(name) != std::string::npos)
            {
                size_t pos = type.find(name);
                std::stringstream in(line);
                std::string type;
                in >> type;
                type = type.substr(pos + name.length());
            }
        }
    }

    return false;
}

void Renderer::ImGuiStartFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;


    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("OpenGLAppDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    ImGui::End();
}

void Renderer::ImGuiSettings()
{
    ImGui::Begin("Settings");
    if (Fullscreen)
    {
        ImGui::Checkbox("Start rendering", &startRendering); ImGui::Text("Before rendering move widgets out of main window");
    }
    if (ImGui::Checkbox("Accumulate", &accumulate))
    {
        frame = 0;
    }
    if (ImGui::InputInt("Samples", &samples))
    {
        accumulate = false;
        if (samples > 30)
            samples = 30;
    }
    if (ImGui::InputInt("Bounces", &bounces))
    {
        accumulate = false;
        if (bounces > 30)
            bounces = 30;
    }


    if (ImGui::Button("Save Image"))
    {
        writeImage = true;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(100.0f);
    ImGui::InputText("Filename", str0, IM_ARRAYSIZE(str0));
    ImGui::PopItemWidth();


    // TODO: Implement a video creation tool.
    /*if (ImGui::Button("Save Video (saves individual frames too"))
    {
        writeVideo = true;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(100.0f);
    if (ImGui::InputText("Filename", str1, IM_ARRAYSIZE(str1)))
    {
        std::cout << str1 << std::endl;
    }
    ImGui::PopItemWidth();*/

    if (filexists)
    {
        ImGui::Text("File already exists");
    }


    ImGui::Text("FPS: %i", frames);
    ImGui::Text(std::string("ms: " + std::to_string(deltaTime)).c_str());
    ImGui::Text("Frame: %i", frame);
    ImGui::End();
}

void Renderer::ImGuiScene()
{
    ImGui::Begin("Scene");

    ImGui::Text("Select scene");
    for (int n = 0; n < m_Scenes.size(); n++)
    {
        if (ImGui::Button(std::string("Scene " + std::to_string(n + 1)).c_str()))
        {
            selectedList = n;
            accumulate = false;
        }
    }

    ImGui::SeparatorText("Scene settings");

    if (ImGui::DragFloat("FOV", &m_Scenes[selectedList].fov, 0.01f))
        accumulate = false;
    if (ImGui::DragFloat3("lookFrom", glm::value_ptr(m_Scenes[selectedList].lookFrom), 0.01f))
        accumulate = false;
    if (ImGui::DragFloat3("lookAt", glm::value_ptr(m_Scenes[selectedList].lookAt), 0.01f))
        accumulate = false;
    if (ImGui::DragFloat3("vUp", glm::value_ptr(m_Scenes[selectedList].vUp), 0.01f))
        accumulate = false;
    if (ImGui::ColorEdit3("Background Color", glm::value_ptr(m_Scenes[selectedList].BackgroundColor), 0.01f))
        accumulate = false;
    if (m_Scenes[selectedList].blur)
    {
        ImGui::DragFloat("focusDist", &m_Scenes[selectedList].focusDist, 0.01f);
        ImGui::DragFloat("defocusAngle", &m_Scenes[selectedList].defocusAngle, 0.01f);
    }

    if(ImGui::Checkbox("Blur", &m_Scenes[selectedList].blur))
        accumulate = false;

    if (ImGui::Button("Add sphere"))
    {
        m_Scenes[selectedList].spheres.emplace_back(Sphere{ glm::vec3(0.0f), 1.0f, glm::vec3(0.0f), 3.0f, 1.0f, 1.5f });
        accumulate = false;
    }

    ImGui::SeparatorText("Spheres");

    for (size_t i = 0; i < m_Scenes[selectedList].spheres.size(); i++)
    {
        Sphere& sphere = m_Scenes[selectedList].spheres[i];
        ImGui::Separator();
        ImGui::PushID(i);
        if (ImGui::DragFloat3("Position", glm::value_ptr(sphere.center), 0.01f))
            accumulate = false;
        ImGui::Separator();
        if (ImGui::Checkbox("MotionBlur", &sphere.isMoving))
            accumulate = false;
        ImGui::Separator();
        if(sphere.isMoving)
            if (ImGui::DragFloat3("Second Position (small values)", glm::value_ptr(sphere.sPosition), 0.0001f))
            accumulate = false;
        ImGui::Separator();
        if (ImGui::ColorEdit3("Color", glm::value_ptr(sphere.color), 0.01f))
            accumulate = false;
        ImGui::Separator();
        ImGui::PushItemWidth(100.0f);
        if (ImGui::DragFloat("Emission Power", &sphere.emissionPower, 0.01f))
            accumulate = false;
        ImGui::Separator();
        if (ImGui::DragFloat("Radius", &sphere.radius, 0.01f))
            accumulate = false;
        ImGui::Separator();
        if (ImGui::InputFloat(indexLabelMap[sphere.index].c_str(), &sphere.index, 1.0f))
            accumulate = false;
        ImGui::PopItemWidth();
        ImGui::Separator();
        if (sphere.index == 1.0f)
        {
            if (ImGui::DragFloat("Fuzziness", &sphere.fuzziness, 0.01f))
                accumulate = false;
        }
        if (sphere.index == 3.0f)
        {
            if (ImGui::DragFloat("Refraction", &sphere.refraction, 0.001f))
                accumulate = false;
        }
        ImGui::Separator();
        if (ImGui::Button("Remove sphere"))
        {
            m_Scenes[selectedList].spheres.erase(m_Scenes[selectedList].spheres.begin() + i);
            accumulate = false;
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    ImGui::End();
}

void Renderer::ImGuiEndFrame()
{
    if (!Fullscreen)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Viewport");
        if (accumulate)
        {
            ImGui::Image((void*)accumulatedImage, ImVec2(IMAGE_WIDTH, IMAGE_HEIGHT));
        }
        else
        {
            ImGui::Image((void*)normalImage, ImVec2(IMAGE_WIDTH, IMAGE_HEIGHT));
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    ImGui::EndFrame();

    //OnResize(ViewportWidth, ViewportHeight);
}


void Renderer::preRender(int width, int height)
{
    aspectRatio = float(width) / float(height);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    float vertices[] = {
        // Positions       // Texture Coords
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // Top Right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // Bottom Right
       -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // Bottom Left
       -1.0f,  1.0f, 0.0f, 0.0f, 1.0f  // Top Left
    };

    unsigned int indices[] = {
        0, 1, 3, // First triangle
        1, 2, 3  // Second triangle
    };

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Image image;
    image.channels = 3;
    image.bitDepth = 32;
    image.width = SCR_WIDTH;
    image.height = SCR_HEIGHT;
    GLenum type = GL_UNSIGNED_BYTE;
    switch (image.bitDepth)
    {
    case 16:
        type = GL_UNSIGNED_SHORT;
        break;
    case 32:
        type = GL_FLOAT;
        break;
    default:
        type = GL_UNSIGNED_BYTE;
    }

    GLenum format = GL_RGB;
    switch (image.channels)
    {
    case 1:
        format = GL_LUMINANCE;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
    default:
        format = GL_RGBA;
        break;
    }

    glGenTextures(1, &normalImage);
    if (normalImage == 0)
    {
        std::cout << "Could not create texture." << std::endl;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalImage);;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

    glBindImageTexture(0, normalImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    if (Fullscreen)
    {
        glBindFragDataLocation(pipeline->m_ShaderID, 0, "color");
        glUniform1i(glGetUniformLocation(pipeline->m_ShaderID, "textureSampler"), 0);
    }

    glGenTextures(1, &accumulatedImage);
    if (accumulatedImage == 0)
    {
        std::cout << "Could not create texture." << std::endl;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumulatedImage);;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

    glBindImageTexture(1, accumulatedImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

bool Renderer::SaveScenes()
{
    std::ofstream scenes;
    scenes.open(filePath, std::ofstream::out | std::ofstream::trunc);
    if (!scenes.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }

    for (const Scene& scene : m_Scenes)
    {
        scenes << "#scene" << std::endl;
        scenes << "lookFrom " << scene.lookFrom.x << " " << scene.lookFrom.y << " " << scene.lookFrom.z << std::endl;
        scenes << "lookAt " << scene.lookAt.x << " " << scene.lookAt.y << " " << scene.lookAt.z << std::endl;
        scenes << "vUp " << scene.vUp.x << " " << scene.vUp.y << " " << scene.vUp.z << std::endl;
        scenes << "defocusAngle " << scene.defocusAngle << std::endl;
        scenes << "focusDist " << scene.focusDist << std::endl;
        scenes << "blur " << scene.blur << std::endl;
        scenes << "fov " << scene.fov << std::endl;
        scenes << "BackgroundCol " << scene.BackgroundColor.x << scene.BackgroundColor.y << scene.BackgroundColor.z << std::endl << std::endl;

        for (const Sphere& sphere : scene.spheres)
        {
            scenes << "#sphere" << std::endl;
            scenes << "pos " << sphere.center.x << " " << sphere.center.y << " " << sphere.center.z << std::endl;
            scenes << "radius " << sphere.radius << std::endl;
            scenes << "color " << sphere.color.x << " " << sphere.color.y << " " << sphere.color.z << std::endl;
            scenes << "sPosition " << sphere.sPosition.x << " " << sphere.sPosition.y << " " << sphere.sPosition.z << std::endl;
            scenes << "isMoving " << sphere.isMoving << std::endl;
            scenes << "Epower " << sphere.emissionPower << std::endl;
            scenes << "index  " << sphere.index << std::endl;
            scenes << "fuzz " << sphere.fuzziness << std::endl;
            scenes << "ref " << sphere.refraction << std::endl << std::endl;
        }
    }

    if (scenes.fail()) {
        std::cerr << "Error writing to file" << std::endl;
        return 1;
    }
    scenes.flush(); 
    scenes.close();
    return true;
}

