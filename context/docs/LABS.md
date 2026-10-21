# Lab digest (the coding style you must copy)

Originals live in `context/reference/Lab1/ … context/reference/Lab4/` (all paths below are relative to `context/reference/`). Read the original file before copying a pattern you haven't used yet.
Common to every lab: one `main.cpp`, globals at the top, GLSL as `R"( … )"` raw strings, a free `compileShader()` function, `glGetUniformLocation` cached once before the loop, `glClearColor(0.1f, 0.1f, 0.15f, 1.0f)`, cleanup with `glDelete*` + `glfwTerminate()` at the end. Comments are short and casual.

## Lab 1 — first triangle (`Lab1/GUC_…pdf`)
- Concept: the pipeline runs on the GPU → (1) allocate GPU memory and upload, (2) tell GPU how to read it, (3) draw.
- NDC: only −1..1 on x/y is visible.
- Steps: `float vertices[]` → `glGenBuffers` / `glBindBuffer(GL_ARRAY_BUFFER)` / `glBufferData(..., GL_STATIC_DRAW)` → `glGenVertexArrays` / `glBindVertexArray` → `glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0)` + `glEnableVertexAttribArray(0)` → loop `glDrawArrays(GL_TRIANGLES, 0, 3); glfwSwapBuffers; glfwPollEvents;`.

## Lab 2 — colors, uniforms, transforms, time (`Lab2/Template/main.cpp`)
- Project: Visual Studio `.sln`/`.vcxproj`; `include/glad/glad.h`, `src/glad.c`, `External/include/GLFW`, `lib/glfw3.lib`, `glm/`.
- Window: `glfwInit` → hints `CONTEXT_VERSION 4.3`, `GLFW_OPENGL_CORE_PROFILE` → `glfwCreateWindow` → `glfwMakeContextCurrent` → `glfwSwapInterval(1)` → `gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)` → `glViewport`.
- Shaders: `#version 430 core`; VS `layout(location=0) in vec3 aPos; layout(location=1) in vec3 aColor; out vec3 vColor; uniform mat4 u_Transform; gl_Position = u_Transform * vec4(aPos,1.0);` FS `uniform vec4 u_Tint; FragColor = vec4(vColor,1.0) * u_Tint;`
- Interleaved vertex `x,y,z, r,g,b` → stride `6*sizeof(float)`, color offset `(void*)(3*sizeof(float))`.
- Uniforms: `glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m))`, `glUniform4f(loc, r,g,b,a)`.
- Transforms with GLM: `glm::mat4(1.0f)` → `glm::translate` → `glm::rotate(m, glm::radians(deg), glm::vec3(0,0,1))` → `glm::scale`. Aspect fix: `glm::scale(m, vec3(1/aspect,1,1))`.
- Time/animation: `deltaTime = glfwGetTime() - lastFrame`; `sin(t)` for oscillating offset, pulse scale, color cycling.

## Lab 3 — EBO, multiple shapes in one VBO, circle, input, picking (`Lab3/main.cpp`, `Lab3/2vs.cpp`, `Lab3/Lab3ExtraNotes.cpp`, `Lab3/Lab 3 notes.pdf`)
- Primitive list (notes): GL_POINTS, GL_LINES/LINE_STRIP/LINE_LOOP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN (circles), adjacency variants, GL_PATCHES.
- **EBO**: share vertices, `GLuint indices[] = {0,2,1, 2,3,4, 4,5,1};` `glGenBuffers(1,&EBO); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); glBufferData(...)` while VAO is bound; draw `glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0)`. Index order matters.
- **Several shapes in one VBO**: `std::vector<float> vertices` + `vertices.insert(vertices.end(), {...})`; remember each shape's `start`/`count`; draw a range with `glDrawArrays(mode, start, count)`.
- **Circle**: center vertex + `segments+1` rim vertices `cosf(a), sinf(a)` (a = 2π·i/segments), `count = segments + 2`, drawn with `GL_TRIANGLE_FAN`; scale/translate with the model matrix.
- **Multiple VAOs/VBOs** (`2vs.cpp`): `GLuint VAO[2], VBO[2]; glGenVertexArrays(2, VAO);` set up each VAO's VBO + attrib pointers before moving to the next; bind `VAO[i]` before its draw.
- **Per-draw uniforms**: same VAO, set `u_Transform` + `u_Highlight` before each draw call.
- FS highlight: `uniform float u_Highlight; vec3 c = mix(vColor, vec3(1,1,0), u_Highlight);` (`mix` blends two colors by a factor).
- **Keyboard**: polled `processInput(window)` with `glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS`, or callback `keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)` registered with `glfwSetKeyCallback`; actions GLFW_PRESS (once), GLFW_REPEAT (held), GLFW_RELEASE (reset logic). Esc → `glfwSetWindowShouldClose(window, true)`.
- **Mouse**: `mouseButtonCallback(window, button, action, mods)` via `glfwSetMouseButtonCallback`; cursor via `glfwGetCursorPos` each frame or `cursor_position_callback` via `glfwSetCursorPosCallback`.
- Screen → NDC: `x = 2*mouseX/screenW - 1`, `y = 1 - 2*mouseY/screenH` (screen y points down).
- **Picking**: transform the shape's corners by its model matrix (`glm::vec2(T * glm::vec4(p,0,1))`), then `pointInTriangle(pt,a,b,c)` using `triSign` (cross-product side test). Hover → highlight.
- Extra notes: `createTriangle()` helper that builds VAO/VBO; hide object (`visible=false`) when it leaves bounds; triangle follows the cursor.

## Lab 4 — camera/MVP and model loading (`Lab4/Lab4Camera.cpp`, `Lab4/Template/*`, `Lab4/Lab Notes.pdf`)
- `Lab4Camera.cpp` (the most useful for us):
  - `struct Mesh { GLuint vao, vbo; GLsizei vertexCount; GLenum drawMode; }`, `Mesh createMesh(const float* data, size_t bytes, GLenum mode)` (uploads once, pos+color attribs), `drawMesh(mesh, locModel, modelMatrix)`, `destroyMesh(mesh)`. ← use this pattern for our quad/circle/triangle building blocks.
  - Shader uniforms `u_Model`, `u_View`, `u_Projection`; `gl_Position = u_Projection * u_View * u_Model * vec4(aPos,1)`.
  - **Orthographic** `glm::ortho(-halfW, halfW, -halfH, halfH, near, far)` (default in this file) vs `glm::perspective`. View `glm::lookAt(pos, pos+front, up)`.
  - Hierarchical transform: translate to centre → rotate → translate out → scale (orbiting cube) = parent/child composition.
  - `framebufferSizeCallback` updates globals + `glViewport`; `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` wireframe toggle; `glEnable(GL_DEPTH_TEST)`; window title via `glfwSetWindowTitle`; constants as `const float` at the top.
- `Template/`: learnopengl classes copied verbatim: `shader.h` (`Shader(vertPath, fragPath)`, `use()`, `setInt/setFloat/setVec2/3/4/setMat4`), `camera.h` (FPS camera), `mesh.h` + `model.h` (Assimp, `stb_image.h`), `model.vert`/`model.frag` (texture_diffuse1). Assimp via `vcpkg.json`.
- Model loading is taught but the project's models must be built from our own detailed geometry (OQ14 default: no sprites/textures for characters). Use Assimp only if the team agrees and the TA allows it.

## Build setups seen
- Labs 2–4: Visual Studio solution, GLAD v1 (`#include <glad/glad.h>`, `gladLoadGLLoader`), GL 4.3, `#version 430 core`.
- Course VS Code starter (`starter/CMakeLists.txt`, `starter/main.cpp`, `starter/install_tools.py`, `Installation Guidelines - VS Code.pdf`): CMake + MinGW + Ninja; `external/glfw-3.5.1`, `external/glm`, `external/glad/include`; GLAD v2 (`#define GLAD_GL_IMPLEMENTATION`, `#include <glad/gl.h>`, `gladLoadGL(glfwGetProcAddress)`), GL 4.0 hint, `#version 330 core`. Setup: install VS Code + Python → run `install_tools.py` → build/run with CMake in VS Code (Launch).
- **This project uses the VS Code starter flavour**: GLAD v2 from `context/external/glad` (`#include <glad/gl.h>`, `gladLoadGL(glfwGetProcAddress)`), GL 3.3 core context, `#version 330 core`. When copying Lab 2–4 or learnopengl code, change `<glad/glad.h>` → `<glad/gl.h>` and `gladLoadGLLoader(...)` → `gladLoadGL(glfwGetProcAddress)`; don't mix the two APIs.
