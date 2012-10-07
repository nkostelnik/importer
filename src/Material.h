#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <string>
#include <vector>
#include <map>

class IMaterialParameter;
class IOutputStream;

class Material {

public:

  void setEffect(const std::string& effectFilePathRelative);

  void addParameter(IMaterialParameter* parameter);

  void addTexture(const std::string& key, const std::string& fileName);

  bool hasTextures() const;

public:

  void write(IOutputStream* writer);

private:

  std::string effectFilePath_;

  std::vector<IMaterialParameter*> parameters_;

  std::map<std::string, std::string> textures_;

};

inline void Material::setEffect(const std::string& effectFilePathRelative) {
  effectFilePath_ = effectFilePathRelative;
}

inline void Material::addParameter(IMaterialParameter* parameter) {
  parameters_.push_back(parameter);
}

inline void Material::addTexture(const std::string& key, const std::string& fileName) {
  textures_[key] = fileName;
}

inline bool Material::hasTextures() const {
  return textures_.size() > 0;
}


#endif