/* Copyright 2013-2017 Matt Tytel
 *
 * helm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * helm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "open_gl_background.h"

#include "common.h"
#include "shaders.h"

#define CHECK_GL_ERROR if (glGetError() != GL_NO_ERROR) MOPO_ASSERT(false);

OpenGLBackground::OpenGLBackground() {
  new_background_ = false;
}

OpenGLBackground::~OpenGLBackground() { }

void OpenGLBackground::init(OpenGLContext& open_gl_context) {
  static const float vertices[] = {
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f
  };

  memcpy(vertices_, vertices, 16 * sizeof(float));

  static const int triangles[] = {
    0, 1, 2,
    2, 3, 0
  };

  open_gl_context.extensions.glGenBuffers(1, &vertex_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(sizeof(vertices)));
  open_gl_context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                          vertices_, GL_STATIC_DRAW);

  open_gl_context.extensions.glGenBuffers(1, &triangle_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);

  GLsizeiptr tri_size = static_cast<GLsizeiptr>(static_cast<size_t>(sizeof(triangles)));
  open_gl_context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_size,
                                          triangles, GL_STATIC_DRAW);

  const char* vertex_shader = Shaders::getShader(Shaders::kBackgroundImageVertex);
  const char* fragment_shader = Shaders::getShader(Shaders::kBackgroundImageFragment);

  image_shader_ = new OpenGLShaderProgram(open_gl_context);

  if (image_shader_->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertex_shader)) &&
      image_shader_->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragment_shader)) &&
      image_shader_->link()) {
    image_shader_->use();
    position_ = new OpenGLShaderProgram::Attribute(*image_shader_, "position");
    texture_coordinates_ = new OpenGLShaderProgram::Attribute(*image_shader_, "tex_coord_in");
    texture_uniform_ = new OpenGLShaderProgram::Uniform(*image_shader_, "texture");
  }
}

void OpenGLBackground::destroy(OpenGLContext& open_gl_context) {
  if (background_.getWidth())
    background_.release();

  image_shader_ = nullptr;
  position_ = nullptr;
  texture_coordinates_ = nullptr;
  texture_uniform_ = nullptr;

  open_gl_context.extensions.glDeleteBuffers(1, &vertex_buffer_);
  open_gl_context.extensions.glDeleteBuffers(1, &triangle_buffer_);
}

void OpenGLBackground::bind(OpenGLContext& open_gl_context) {
  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);
  background_.bind();
}

void OpenGLBackground::enableAttributes(OpenGLContext& open_gl_context) {
  CHECK_GL_ERROR
  if (position_ != nullptr) {
    open_gl_context.extensions.glVertexAttribPointer(position_->attributeID, 2, GL_FLOAT,
                                                     GL_FALSE, 4 * sizeof(float), 0);
    open_gl_context.extensions.glEnableVertexAttribArray(position_->attributeID);
  }
  CHECK_GL_ERROR
  if (texture_coordinates_ != nullptr) {
    open_gl_context.extensions.glVertexAttribPointer(texture_coordinates_->attributeID, 2, GL_FLOAT,
                                                     GL_FALSE, 4 * sizeof(float),
                                                     (GLvoid*)(2 * sizeof(float)));
    open_gl_context.extensions.glEnableVertexAttribArray(texture_coordinates_->attributeID);
  }
  CHECK_GL_ERROR
}

void OpenGLBackground::disableAttributes(OpenGLContext& open_gl_context) {
  CHECK_GL_ERROR
  if (position_ != nullptr)
    open_gl_context.extensions.glDisableVertexAttribArray(position_->attributeID);
  CHECK_GL_ERROR
  if (texture_coordinates_ != nullptr)
    open_gl_context.extensions.glDisableVertexAttribArray(texture_coordinates_->attributeID);
  CHECK_GL_ERROR
}

void OpenGLBackground::render(OpenGLContext& open_gl_context) {
  CHECK_GL_ERROR

  if (new_background_ && background_image_.getWidth() > 0) {
    new_background_ = false;
    background_.loadImage(background_image_);

    CHECK_GL_ERROR

    float width_ratio = (1.0f * background_.getWidth()) / background_image_.getWidth();
    float height_ratio = (1.0f * background_.getHeight()) / background_image_.getHeight();
    float width_end = 2.0f * width_ratio - 1.0f;
    float height_end = 1.0f - 2.0f * height_ratio;

    vertices_[8] = vertices_[12] = width_end;
    vertices_[5] = vertices_[9] = height_end;

    open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(16 * sizeof(float)));
    open_gl_context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                            vertices_, GL_STATIC_DRAW);

    CHECK_GL_ERROR
  }

  CHECK_GL_ERROR

  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  CHECK_GL_ERROR

  image_shader_->use();
  bind(open_gl_context);
  open_gl_context.extensions.glActiveTexture(GL_TEXTURE0);
  background_.bind();

  CHECK_GL_ERROR

  if (texture_uniform_ != nullptr && background_.getWidth())
    texture_uniform_->set(0);

  CHECK_GL_ERROR
  enableAttributes(open_gl_context);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  disableAttributes(open_gl_context);

  CHECK_GL_ERROR

  background_.unbind();

  CHECK_GL_ERROR

  open_gl_context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl_context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OpenGLBackground::updateBackgroundImage(Image background) {
  background_image_ = background;
  new_background_ = true;
}