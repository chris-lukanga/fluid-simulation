# Shader Module Documentation

## Overview
The `shader` module provides a C++20 module-based implementation for managing OpenGL shader programs. It encapsulates shader compilation, linking, and uniform variable management into a convenient `Shader` class.

---

## Functions

### `readFileToString(const std::string& filePath)`

**Description:**
Reads a shader file from disk and returns its contents as a string. Automatically removes UTF-8 Byte Order Mark (BOM) if present, which is essential for proper shader compilation.

**Parameters:**
- `filePath` (const std::string&): The relative or absolute path to the shader file to read.

**Returns:**
- std::string: The contents of the file with BOM removed. Returns an empty string if the file cannot be opened.

**Example Usage:**
```cpp
std::string vertexShader = readFileToString("shaders/vertexShader.glsl");
std::string fragmentShader = readFileToString("shaders/fragmentShader.glsl");
```

**Notes:**
- Reads files in binary mode to preserve exact byte content
- Automatically detects and removes UTF-8 BOM (bytes 0xEF, 0xBB, 0xBF)
- Prints an error message to stderr if the file cannot be opened
- This function is critical for resolving shader compilation errors related to illegal characters at the start of shader code

---

### `checkCompileErrors(unsigned int shader, const std::string& type)`

**Description:**
Checks for compilation or linking errors in OpenGL shader objects or programs. Logs detailed error messages to stderr if errors are found.

**Parameters:**
- `shader` (unsigned int): The OpenGL shader or program ID to check.
- `type` (const std::string&): The type of compilation to check. Valid values:
  - `"VERTEX"`: Check vertex shader compilation
  - `"FRAGMENT"`: Check fragment shader compilation
  - `"PROGRAM"`: Check program linking

**Returns:**
- void

**Example Usage:**
```cpp
unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(vertexShader, 1, &vShaderCode, nullptr);
glCompileShader(vertexShader);
checkCompileErrors(vertexShader, "VERTEX");
```

**Error Output Format:**
```
ERROR::SHADER_COMPILATION_ERROR of type: [TYPE]
[Detailed OpenGL error message]
 -- --------------------------------------------------- --
```

**Notes:**
- Only logs output if an error is detected; successful compilations are silent
- Provides detailed error information from OpenGL's error log
- Essential for debugging shader compilation issues

---

## Classes

### `Shader` (Export Class)

**Description:**
A wrapper class for OpenGL shader programs that handles shader compilation, linking, and uniform variable management. Simplifies the process of creating and using shader programs in OpenGL applications.

**Member Variables:**
- `unsigned int ID`: The OpenGL program ID associated with this shader

---

#### Constructor

##### `Shader(const char* vertexPath, const char* fragmentPath)`

**Description:**
Creates a new Shader object by loading, compiling, and linking vertex and fragment shaders from disk.

**Parameters:**
- `vertexPath` (const char*): Path to the vertex shader source file
- `fragmentPath` (const char*): Path to the fragment shader source file

**Process:**
1. Reads both shader files using `readFileToString()`
2. Creates and compiles the vertex shader
3. Creates and compiles the fragment shader
4. Links both shaders into a program
5. Deletes the individual shader objects (they're no longer needed after linking)
6. Checks for compilation and linking errors at each step

**Example Usage:**
```cpp
Shader shader("shaders/vertexShader.glsl", "shaders/fragmentShader.glsl");
```

**Exception Behavior:**
- Does not throw exceptions; errors are logged to stderr via `checkCompileErrors()`
- If compilation fails, the shader program will still be created but may not render correctly

---

#### `void use()`

**Description:**
Activates this shader program for rendering. All subsequent OpenGL draw calls will use this shader.

**Parameters:**
- None

**Returns:**
- void

**Example Usage:**
```cpp
shader.use();
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
```

**Notes:**
- Must be called before any draw commands that use this shader
- You only need to call `use()` once unless you're switching between multiple shaders

---

#### `void setBool(const std::string &name, bool value) const`

**Description:**
Sets a boolean uniform variable in the shader program.

**Parameters:**
- `name` (const std::string&): The name of the uniform variable in the shader code
- `value` (bool): The value to set (true or false)

**Returns:**
- void

**Example Usage:**
```cpp
shader.use();
shader.setBool("enableTexture", true);
```

**Notes:**
- Boolean values are converted to integers (true = 1, false = 0)
- The shader must be active (use() called) for the uniform to be set correctly

---

#### `void setInt(const std::string &name, int value) const`

**Description:**
Sets an integer uniform variable in the shader program.

**Parameters:**
- `name` (const std::string&): The name of the uniform variable in the shader code
- `value` (int): The integer value to set

**Returns:**
- void

**Example Usage:**
```cpp
shader.use();
shader.setInt("textureUnit", 0);
```

**Notes:**
- Used for integer uniforms, texture unit indices, and other integer values
- The shader must be active for the uniform to be set correctly

---

#### `void setFloat(const std::string &name, float value) const`

**Description:**
Sets a floating-point uniform variable in the shader program.

**Parameters:**
- `name` (const std::string&): The name of the uniform variable in the shader code
- `value` (float): The floating-point value to set

**Returns:**
- void

**Example Usage:**
```cpp
shader.use();
float timeValue = static_cast<float>(glfwGetTime());
shader.setFloat("time", timeValue);
```

**Notes:**
- Used for time values, scaling factors, and other floating-point parameters
- The shader must be active for the uniform to be set correctly

---

## Complete Usage Example

```cpp
// Create shader program
Shader shader("shaders/vertexShader.glsl", "shaders/fragmentShader.glsl");

// In render loop
while (!glfwWindowShouldClose(window)) {
    // Activate shader
    shader.use();
    
    // Set uniforms
    float timeValue = static_cast<float>(glfwGetTime());
    shader.setFloat("time", timeValue);
    shader.setInt("textureUnit", 0);
    shader.setBool("useColor", true);
    
    // Draw
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

---

## File Structure

```
shader.cpp
├── Module declarations
├── Utility functions
│   ├── readFileToString()
│   └── checkCompileErrors()
└── Shader class
    ├── Constructor
    ├── use()
    ├── setBool()
    ├── setInt()
    └── setFloat()
```

---

## Common Issues and Solutions

### Shader Compilation Errors
**Problem:** "Illegal character" or syntax errors in shader
- **Solution:** Ensure shader files don't have UTF-8 BOM. The `readFileToString()` function handles this automatically.

### Uniforms Not Updating
**Problem:** Setting uniforms has no effect on rendering
- **Solution:** Make sure to call `shader.use()` before setting uniforms.

### File Not Found
**Problem:** "Failed to open shader file" message
- **Solution:** Verify shader file paths are correct relative to the working directory where the program runs.

---

## C++20 Module Features

This implementation uses C++20 modules for better encapsulation:
- The `Shader` class is exported from the module
- Utility functions are module-internal
- Cleaner dependency management compared to traditional headers

**Import Usage:**
```cpp
import shader;

// Now you can use the Shader class
Shader myShader("vertex.glsl", "fragment.glsl");
```
