import bpy
import mathutils
import math
import os
import sys
import base64
import xml.etree.cElementTree as et


def Clamp(value, min_value, max_value):
    return max(min(value, max_value), min_value)


def BoolToString(obj):
    if obj:
        return "true"
    else:
        return "false"


def FloatToString(obj):
    return '{:g}'.format(obj)


def QuaternionToString(obj):
    return '[{:g},{:g},{:g},{:g}]'.format(obj.w, obj.x, obj.y, obj.z)


def Vector4ToString(obj):
    return '[{:g},{:g},{:g},{:g}]'.format(obj.x, obj.y, obj.z, obj.w)


def Matrix4ToString(obj):
    return '[{:s} {:s} {:s} {:s}]'.format(Vector4ToString(obj[0]),
                                          Vector4ToString(obj[1]),
                                          Vector4ToString(obj[2]),
                                          Vector4ToString(obj[3]))


def Vector3ToString(obj):
    return '[{:g},{:g},{:g}]'.format(obj.x, obj.y, obj.z)


def Vector2ToString(obj):
    return '[{:g},{:g}]'.format(obj.x, obj.y)


def ColorToString(obj):
    return '[{:g},{:g},{:g}]'.format(obj.r, obj.g, obj.b)


def BoundTypeToString(bound):
    if bound == 'BOX':
        return "SH_BOX"
    elif bound == 'SPHERE':
        return "SH_SPHERE"
    elif bound == 'CYLINDER':
        return "SH_CYLINDER_Z"
    elif bound == 'CONE':
        return "SH_CONE_Z"
    elif bound == 'CONVEX_HULL':
        return "SH_CONVEX_TRIMESH"
    elif bound == 'TRIANGLE_MESH':
        return "SH_TRIMESH"
    elif bound == 'CAPSULE':
        return "SH_CAPSULE_Z"
    else:
        return "SH_UNKNOWN"


def LightTypeToString(lightType):
    if lightType == 'SUN':
        return "DIRECTIONAL"
    elif lightType == 'SPOT':
        return "SPOT"
    else:
        return "POINT"


def SensorFitToString(sensorFit):
    if sensorFit == 'AUTO':
        return "0"
    elif sensorFit == 'HORIZONTAL':
        return "1"
    else:
        return "2"


QUATERNION_IDENTITY = mathutils.Quaternion()
QUATERNION_IDENTITY.identity()
VECTOR3_ZERO = mathutils.Vector((0, 0, 0))
VECTOR4_ZERO = mathutils.Vector((0, 0, 0, 0))
VECTOR3_ONE = mathutils.Vector((1, 1, 1))
DEFAULT_MATERIAL_NAME = "NSGDefault"


def ConvertGroup(parentEle, group):
    print("Converting group " + group.name)
    for obj in group.objects:
        if type(obj) is bpy.types.Group:
            ConvertGroup(parentEle, obj)
        else:
            ConvertObject(parentEle, obj)


def CreateSceneNode(name, parentElem, obj, materialIndex=-1, loc=None, rot=None, sca=None):
    sceneNodeEle = et.SubElement(parentElem, "SceneNode")
    sceneNodeEle.set("name", name)

    if loc and loc != VECTOR3_ZERO:
        sceneNodeEle.set("position", Vector3ToString(loc))
    if rot and rot != QUATERNION_IDENTITY:
        sceneNodeEle.set("orientation", QuaternionToString(rot))
    if sca and sca != VECTOR3_ONE:
        sceneNodeEle.set("scale", Vector3ToString(sca))

    if obj and obj.hide_render:
        sceneNodeEle.set("hide", BoolToString(True))

    if materialIndex < 1:
        CreatePhysics(sceneNodeEle, obj, materialIndex)
    if obj and obj.dupli_type == 'GROUP' and obj.dupli_group:
            ConvertGroup(sceneNodeEle, obj.dupli_group)
    return sceneNodeEle


def ConvertUVMaps(meshEle, mesh):
    for index, uv in enumerate(mesh.tessface_uv_textures):
        key = "uv" + str(index) + "Name"
        meshEle.set(key, uv.name)


def IsBoneDeformGroup(armatureObj, deformGroup):
    for bone in armatureObj.pose.bones:
        if deformGroup.name == bone.name:
            return True
    return False


def GetBoneIndexByDeformGroup(meshObj, armatureObj):
    result = []
    i = 0
    for deformGroup in meshObj.vertex_groups:
        data = [-1, deformGroup.name]
        if IsBoneDeformGroup(armatureObj, deformGroup):
            data[0] = i
            i += 1
        result.append(data)
    return result


def ConvertBonesWeigths(jointList, vertexData, vertex):
    total_groups = Clamp(len(vertex.groups), 0, 4)
    if total_groups > 0:
        bonesID = mathutils.Vector((0.0, 0.0, 0.0, 0.0))
        bonesWeight = mathutils.Vector((0.0, 0.0, 0.0, 0.0))
        for i in range(total_groups):
            group = vertex.groups[i]
            if group.group >= len(jointList):
                continue
            joint_index = jointList[group.group][0]
            if joint_index != -1:
                bonesID[i] = joint_index
                bonesWeight[i] = group.weight

        vertexData["b"] = Vector4ToString(bonesID)
        vertexData["w"] = Vector4ToString(bonesWeight)


def ConvertMesh(name, meshesEle, meshObj, materialIndex):
    meshEle = GetChildEle(meshesEle, "Mesh", "name", name)
    if meshEle is not None:
        return
    mesh = meshObj.data
    if len(mesh.vertices) == 0:
        return
    print("Converting mesh " + mesh.name)
    mesh.update(calc_tessface=True)
    meshEle = et.SubElement(meshesEle, "Mesh")
    meshEle.set("name", name)
    ConvertUVMaps(meshEle, mesh)
    armatureObj = meshObj.parent
    jointList = []
    if armatureObj and armatureObj.type == 'ARMATURE':
        jointList = GetBoneIndexByDeformGroup(meshObj, armatureObj)
    indexes = ""
    i = 0
    vertexesEle = et.SubElement(meshEle, "Vertexes")
    indexesEle = et.SubElement(meshEle, "Indexes")
    for iface, face in enumerate(mesh.tessfaces):
        if face.material_index != materialIndex:
            continue
        assert len(face.vertices) == 4 or len(face.vertices) == 3
        if len(face.vertices) == 4:
            indexes = indexes + '{:d} {:d} {:d} {:d} {:d} {:d} '.format(i, i+1, i+2, i, i+2, i+3)
        else:
            indexes = indexes + '{:d} {:d} {:d} '.format(i, i+1, i+2)
        i += len(face.vertices)
        vertexNum = 0
        for ivertex in face.vertices:
            vertex = mesh.vertices[ivertex]
            vertexData = {}
            vertexData["p"] = Vector3ToString(vertex.co)
            if face.use_smooth:
                vertexData["n"] = Vector3ToString(vertex.normal)
            else:
                vertexData["n"] = Vector3ToString(face.normal)
            ConvertBonesWeigths(jointList, vertexData, vertex)

            channels = Clamp(len(mesh.tessface_uv_textures), 0, 2)
            keys = ("u", "v")
            for channel in range(channels):
                uv_channel = mesh.tessface_uv_textures[channel]
                key = keys[channel]
                if vertexNum == 0:
                    vertexData[key] = Vector2ToString(uv_channel.data[iface].uv1)
                elif vertexNum == 1:
                    vertexData[key] = Vector2ToString(uv_channel.data[iface].uv2)
                elif vertexNum == 2:
                    vertexData[key] = Vector2ToString(uv_channel.data[iface].uv3)
                elif vertexNum == 3:
                    vertexData[key] = Vector2ToString(uv_channel.data[iface].uv4)

            channels = Clamp(len(mesh.tessface_vertex_colors), 0, 1)
            for channel in range(channels):
                vc_channel = mesh.tessface_vertex_colors[channel]
                if vertexNum == 0:
                    vertexData["c"] = ColorToString(vc_channel.data[iface].color1)
                elif vertexNum == 1:
                    vertexData["c"] = ColorToString(vc_channel.data[iface].color2)
                elif vertexNum == 2:
                    vertexData["c"] = ColorToString(vc_channel.data[iface].color3)
                elif vertexNum == 3:
                    vertexData["c"] = ColorToString(vc_channel.data[iface].color4)

            vertexNum += 1
            vertexDataEle = et.SubElement(vertexesEle, "VertexData")
            for k, v in vertexData.items():
                vertexDataEle.set(k, v)
    indexesEle.text = indexes


def ConvertTexture(materialEle, textureSlot):
    if textureSlot is None:
        return
    texture = textureSlot.texture
    if texture.type != 'IMAGE':
        return
    image = texture.image
    if not image:
        return
    textureEle = et.SubElement(materialEle, "Texture")
    textureEle.set("uvName", textureSlot.uv_layer)
    textureEle.set("resource", "data/" + image.name)
    textureEle.set("useAlpha", BoolToString(image.use_alpha))
    if texture.extension == 'REPEAT':
        textureEle.set("wrapMode", "REPEAT")
    elif texture.extension == 'CHECKER':
        textureEle.set("wrapMode", "MIRRORED_REPEAT")
    else:
        textureEle.set("wrapMode", "CLAMP_TO_EDGE")

    if textureSlot.blend_type == 'MIX':
        textureEle.set("blendType", "MIX")
    elif textureSlot.blend_type == 'ADD':
        textureEle.set("blendType", "ADD")
    elif textureSlot.blend_type == 'SUBTRACT':
        textureEle.set("blendType", "SUB")
    elif textureSlot.blend_type == 'MULTIPLY':
        textureEle.set("blendType", "MUL")
    elif textureSlot.blend_type == 'SCREEN':
        textureEle.set("blendType", "SCREEN")
    elif textureSlot.blend_type == 'OVERLAY':
        textureEle.set("blendType", "OVERLAY")
    elif textureSlot.blend_type == 'DIFFERENCE':
        textureEle.set("blendType", "DIFF")
    elif textureSlot.blend_type == 'DIVIDE':
        textureEle.set("blendType", "DIV")
    elif textureSlot.blend_type == 'DARKEN':
        textureEle.set("blendType", "DARK")
    elif textureSlot.blend_type == 'LIGHTEN':
        textureEle.set("blendType", "LIGHT")
    elif textureSlot.blend_type == 'HUE':
        textureEle.set("blendType", "BLEND_HUE")
    elif textureSlot.blend_type == 'SATURATION':
        textureEle.set("blendType", "BLEND_SAT")
    elif textureSlot.blend_type == 'VALUE':
        textureEle.set("blendType", "BLEND_VAL")
    elif textureSlot.blend_type == 'COLOR':
        textureEle.set("blendType", "BLEND_COLOR")
    elif textureSlot.blend_type == 'SOFT_LIGHT':
        textureEle.set("blendType", "LIGHT")
    elif textureSlot.blend_type == 'LINEAR_LIGHT':
        textureEle.set("blendType", "LIGHT")
    else:
        textureEle.set("blendType", "MIX")

    if textureSlot.use_map_ambient:
        textureEle.set("mapType", "AMB")
    elif textureSlot.use_map_color_diffuse or textureSlot.use_map_diffuse:
        textureEle.set("mapType", "COL")
    elif textureSlot.use_map_normal:
        textureEle.set("mapType", "NORM")
    elif textureSlot.use_map_color_spec or textureSlot.use_map_specular:
        textureEle.set("mapType", "SPEC")
    elif textureSlot.use_map_emit:
        textureEle.set("mapType", "EMIT")
    elif textureSlot.use_map_color_reflection:
        textureEle.set("mapType", "COLMIR")
    elif textureSlot.use_map_alpha:
        textureEle.set("mapType", "ALPHA")
    else:
        textureEle.set("mapType", "COL")

    if texture.filter_type == 'BOX':
        textureEle.set("filterMode", "NEAREST")
    else:
        textureEle.set("filterMode", "BILINEAR")

    textureEle.set("flags", "11")
    textureEle.set("flagNames", " INVERT_Y GENERATE_MIPMAPS")
    uvTransform = mathutils.Vector((textureSlot.scale.x, textureSlot.scale.y, textureSlot.offset.x, textureSlot.offset.y))
    textureEle.set("uvTransform", Vector4ToString(uvTransform))


def ConvertMaterial(materialsEle, material):
    print("Converting material " + material.name)
    materialEle = et.SubElement(materialsEle, "Material")
    alpha = 1  # full opaque
    alphaForSpecular = 1  # full opaque
    transparent = material.use_transparency
    if transparent:
        alpha = material.alpha
        alphaForSpecular = material.specular_alpha
    materialEle.set("name", material.name)
    materialEle.set("shadeless", BoolToString(material.use_shadeless))
    materialEle.set("castShadow", BoolToString(material.use_cast_shadows))
    materialEle.set("receiveShadows", BoolToString(material.use_shadows))
    if material.game_settings.use_backface_culling and not transparent:
        materialEle.set("cullFaceMode", "BACK")
    elif material.game_settings.invisible and not transparent:
        materialEle.set("cullFaceMode", "FRONT_AND_BACK")
    else:
        materialEle.set("cullFaceMode", "DISABLED")
    # materialEle.set("friction", ???)
    materialEle.set("ambientIntensity", FloatToString(material.ambient))
    materialEle.set(
        "diffuseIntensity", FloatToString(material.diffuse_intensity))
    materialEle.set(
        "specularIntensity", FloatToString(material.specular_intensity))
    materialEle.set("diffuse", ColorToString(material.diffuse_color))
    materialEle.set("specular", ColorToString(material.specular_color))
    materialEle.set("shininess", str(material.specular_hardness))
    materialEle.set("emitIntensity", str(material.emit))
    if material.type == 'WIRE':
        materialEle.set("fillMode", "WIREFRAME")
    else:
        materialEle.set("fillMode", "SOLID")
    materialEle.set("alpha", FloatToString(alpha))
    materialEle.set("alphaForSpecular", FloatToString(alphaForSpecular))
    materialEle.set("isTransparent", BoolToString(transparent))
    if material.shadow_buffer_bias == 0:
        materialEle.set("shadowBias", "0.001")
    else:
        materialEle.set("shadowBias", FloatToString(material.shadow_buffer_bias))
    if material.game_settings.text:
        materialEle.set("renderPass", "TEXT")
    elif material.use_vertex_color_paint:
        materialEle.set("renderPass", "VERTEXCOLOR")
    elif material.use_shadeless:
        materialEle.set("renderPass", "UNLIT")
    else:
        materialEle.set("renderPass", "LIT")
    # FIX Engine (Load/Save)
    if material.game_settings.face_orientation == 'HALO':
        materialEle.set("billboardType", "SPHERICAL")
    elif material.game_settings.face_orientation == 'BILLBOARD':
        materialEle.set("billboardType", "CYLINDRICAL")
    else:
        materialEle.set("billboardType", "NONE")

    for index, textureSlot in enumerate(material.texture_slots):
        if material.use_textures[index]:
            ConvertTexture(materialEle, textureSlot)


def ConvertMaterials():
    materialsEle = et.SubElement(appEle, "Materials")
    for material in bpy.data.objects.data.materials:
        ConvertMaterial(materialsEle, material)


def GetArmatureMeshObj(armatureObj):
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.vertex_groups and obj.parent == armatureObj:
            return obj
    return None


def BuildBonetree(parentEle, bone):
    boneEle = et.SubElement(parentEle, "Bone")
    boneEle.set("name", bone.name)

    parBind = mathutils.Matrix()
    if bone.parent:
        parBind = bone.parent.matrix_local.inverted()

    bind = parBind * bone.matrix_local
    loc, rot, sca = bind.decompose()

    if loc != VECTOR3_ZERO:
        boneEle.set("position", Vector3ToString(loc))
    if rot != QUATERNION_IDENTITY:
        boneEle.set("orientation", QuaternionToString(rot))
    if sca != VECTOR3_ONE:
        boneEle.set("scale", Vector3ToString(sca))

    for child in bone.children:
        BuildBonetree(boneEle, child)


def ConvertPoseBone(shaderOrderEle, poseBone):
    boneEle = et.SubElement(shaderOrderEle, "Bone")
    boneEle.set("name", poseBone.name)
    m = poseBone.matrix.inverted().transposed()
    boneEle.set("offsetMatrix", Matrix4ToString(m))


def ConvertArmature(armaturesEle, armatureObj):
    meshObj = GetArmatureMeshObj(armatureObj)
    if meshObj is None:
        return
    armature = armatureObj.data
    armatureEle = GetChildEle(armaturesEle, "Skeleton", "name", armature.name)
    if armatureEle is None:
        print("Converting armature " + armature.name)
        poseStatus = armature.pose_position
        armature.pose_position = 'REST'
        bpy.context.scene.update()
        armatureEle = et.SubElement(armaturesEle, "Skeleton")
        armatureEle.set("name", armature.name)

        shaderOrderEle = et.SubElement(armatureEle, "ShaderOrder")
        for deformGroup in meshObj.vertex_groups:
            for bone in armatureObj.pose.bones:
                if deformGroup.name == bone.name:
                    ConvertPoseBone(shaderOrderEle, bone)

        armature = armatureObj.data
        BonesEle = et.SubElement(armatureEle, "Bones")
        for bone in armature.bones:
            if not bone.parent:
                BuildBonetree(BonesEle, bone)

        armature.pose_position = poseStatus
        bpy.context.scene.update()


def ConvertImage(resourcesEle, image, embed):
    if image.users > 0 and image.packed_file:
        if image.source == 'FILE':
            resourceEle = et.SubElement(resourcesEle, "Resource")
            resourceEle.set("name", "data/" + image.name)
            imagePath = datadir + "/" + image.name
            print("Saving " + imagePath)
            image.save_render(imagePath)
            if embed:
                dataEle = et.SubElement(resourceEle, "data")
                image_file = open(imagePath, "rb")
                encoded_string = base64.b64encode(image_file.read())
                string = str(encoded_string[2:-1])
                dataEle.set("dataSize", str(len(string)))
                # print(string)
                dataEle.text = string
                image_file.close()


def ConvertSound(resourcesEle, soundsEle, sound, embed):
    if sound.users > 0 and sound.packed_file:
        resourceEle = et.SubElement(resourcesEle, "Resource")
        resourceName = "data/" + sound.filepath
        resourceEle.set("name", resourceName)
        soundPath = datadir + "/" + sound.filepath
        print("Saving " + soundPath)
        soundfile = open(soundPath, "wb")
        soundfile.write(sound.packed_file.data)
        soundfile.close()
        soundEle = et.SubElement(soundsEle, "Sound")
        soundEle.set("name", sound.name)
        soundEle.set("resource", resourceName)


def ConvertResources(embed):
    resourcesEle = et.SubElement(appEle, "Resources")
    for image in bpy.data.objects.data.images:
        ConvertImage(resourcesEle, image, embed)
    soundsEle = et.SubElement(appEle, "Sounds")
    for sound in bpy.data.objects.data.sounds:
        ConvertSound(resourcesEle, soundsEle, sound, embed)


def ExtractTransform(obj):
    return obj.matrix_local.decompose()


def ExtractInvertedTransform(obj):
    return obj.matrix_local.inverted().decompose()


def GetChildEle(ele, childEleName, attName, attNameValue):
    childEle = ele.findall(
        ".//" + childEleName + "[@" + attName + "='" + attNameValue + "']")
    if len(childEle) == 1:
        return childEle[0]
    return None


def GetParentEle(eleName, attName, attNameValue):
    tree = et.ElementTree(appEle)
    parentEle = tree.findall(
        ".//" + eleName + "[@" + attName + "='" + attNameValue + "']/..")
    if len(parentEle) == 1:
        return parentEle[0]
    return None


def GetOrCreateChildEle(ele, childEleName, attName, attNameValue):
    childEle = GetChildEle(ele, childEleName, attName, attNameValue)
    if childEle is None:
        childEle = et.SubElement(ele, childEleName)
        childEle.set(attName, attNameValue)
    return childEle


def GetMesh(name):
    for mesh in bpy.data.objects.data.meshes:
        if mesh.name == name:
            return mesh
    return None


def GetObjMaterialName(name, obj, materialIndex):
    if materialIndex > 0:
        materialSlot = obj.material_slots[materialIndex]
        return name + "_" + materialSlot.name
    return name


def ConvertMeshObject(meshesEle, parentEle, obj):
    materialIndex = -1
    nMaterials = len(obj.material_slots)
    if nMaterials > 0:
        materialIndex = 0
    position, rotation, scale = ExtractTransform(obj)
    parentEle = sceneNodeEle = CreateSceneNode(
        obj.name, parentEle, obj, materialIndex, position, rotation, scale)
    sceneNodeEle.set("meshName", obj.data.name)
    if nMaterials > 0:
        materialSlot = obj.material_slots[0]
        sceneNodeEle.set("materialName", materialSlot.name)
    else:
        sceneNodeEle.set("materialName", DEFAULT_MATERIAL_NAME)
    materialIndex = 1
    while nMaterials > materialIndex:
        materialSlot = obj.material_slots[materialIndex]
        newNodeName = GetObjMaterialName(obj.name, obj, materialIndex)
        sceneNodeEle = CreateSceneNode(
            newNodeName, parentEle, obj, materialIndex, None, None, None)
        meshName = GetObjMaterialName(obj.data.name, obj, materialIndex)
        ConvertMesh(
            meshName, meshesEle, obj, materialIndex)
        sceneNodeEle.set("meshName", meshName)
        if HasRigidBody(obj):
            shapesEle = parentEle.find("RigidBody").find("Shapes")
            if shapesEle:
                shapeEle = et.SubElement(shapesEle, "Shape")
                name, scaleStr, typeStr = GetPhysicShapeName(obj, meshName)
                shapeEle.set("name", name)
        elif HasCharacter(obj):
            shapesEle = parentEle.find("Character").find("Shapes")
            if shapesEle:
                shapeEle = et.SubElement(shapesEle, "Shape")
                name, scaleStr, typeStr = GetPhysicShapeName(obj, meshName)
                shapeEle.set("name", name)
        sceneNodeEle.set("materialName", materialSlot.name)
        materialIndex += 1


def ConvertArmatureObject(armaturesEle, parentEle, armatureObj):
    armature = armatureObj.data
    armatureEle = GetChildEle(armaturesEle, "Skeleton", "name", armature.name)
    if armatureEle is None:
        print("Armature {:s} not found!!!".format(armature.name))
        return

    position, rotation, scale = ExtractTransform(armatureObj)
    sceneNodeEle = CreateSceneNode(
        armatureObj.name, parentEle, armatureObj, -1, position, rotation, scale)
    sceneNodeEle.set("skeleton", armature.name)


def ConvertLampObject(parentEle, obj):
    light = obj.data
    if light.type == 'HEMI':
        return
    position, rotation, scale = ExtractTransform(obj)
    sceneNodeEle = CreateSceneNode(
        obj.name, parentEle, obj, -1, position, rotation, scale)
    sceneNodeEle.set("nodeType", "Light")
    sceneNodeEle.set("type", LightTypeToString(light.type))
    if light.type == 'SPOT':
        cutoff = math.degrees(light.spot_size)
        sceneNodeEle.set("spotCutOff", FloatToString(cutoff))
    sceneNodeEle.set("energy", FloatToString(light.energy))
    sceneNodeEle.set("color", ColorToString(light.color))
    sceneNodeEle.set("diffuse", BoolToString(light.use_diffuse))
    sceneNodeEle.set("specular", BoolToString(light.use_specular))
    sceneNodeEle.set("distance", FloatToString(light.distance))
    sceneNodeEle.set("shadows", BoolToString(light.shadow_method != 'NOSHADOW'))
    sceneNodeEle.set("shadowColor", ColorToString(light.shadow_color))
    sceneNodeEle.set("onlyShadow", BoolToString(light.use_only_shadow))
    sceneNodeEle.set("shadowBias", FloatToString(light.shadow_buffer_bias))
    sceneNodeEle.set(
        "shadowClipStart", FloatToString(light.shadow_buffer_clip_start))
    sceneNodeEle.set(
        "shadowClipEnd", FloatToString(light.shadow_buffer_clip_end))


def ConvertCameraObject(parentEle, obj):
    position, rotation, scale = ExtractTransform(obj)
    sceneNodeEle = CreateSceneNode(
        obj.name, parentEle, obj, -1, position, rotation, scale)
    sceneNodeEle.set("nodeType", "Camera")
    camera = obj.data
    sceneNodeEle.set("zNear", FloatToString(camera.clip_start))
    sceneNodeEle.set("zFar", FloatToString(camera.clip_end))
    if camera.type == 'ORTHO':
        sceneNodeEle.set("isOrtho", BoolToString(True))
    else:
        sceneNodeEle.set("isOrtho", BoolToString(False))
    sceneNodeEle.set("orthoScale", FloatToString(camera.ortho_scale))
    sceneNodeEle.set("sensorFit", SensorFitToString(camera.sensor_fit))

    fov = 2 * math.atan(0.5 * camera.sensor_width / camera.lens)
    if camera.sensor_fit == 'VERTICAL':
        fov = 2 * math.atan(0.5 * camera.sensor_height / camera.lens)
    sceneNodeEle.set("fovy", FloatToString(fov))


def ConvertObject(parentEle, obj):
    print("Converting object " + obj.name + " type " + obj.type)
    if obj.type == 'MESH':
        meshesEle = appEle.find("Meshes")
        ConvertMeshObject(meshesEle, parentEle, obj)
    elif obj.type == 'ARMATURE':
        armaturesEle = appEle.find("Skeletons")
        ConvertArmatureObject(armaturesEle, parentEle, obj)
    elif obj.type == 'LAMP':
        ConvertLampObject(parentEle, obj)
    elif obj.type == 'CAMERA':
        ConvertCameraObject(parentEle, obj)
    else:
        position, rotation, scale = ExtractTransform(obj)
        CreateSceneNode(obj.name, parentEle, obj, -1, position, rotation, scale)


def GetChannelMask(transform_name):
    if transform_name == "location":
        return 1 << 0
    elif transform_name == "rotation_quaternion":
        return 1 << 1
    elif transform_name == "scale":
        return 1 << 2
    else:
        return 0


def GetPathData(path):
    pathType = path[:10]
    chan_name = path[path.find('"') + 1: path.rfind('"')]
    transform_name = path[path.rfind('.') + 1:]
    return pathType, chan_name, transform_name


def ConvertTransform(keyframeEle, transforms):
    loc = mathutils.Vector((0, 0, 0))
    rot = mathutils.Quaternion()
    rot.identity()
    sca = mathutils.Vector((1, 1, 1))
    for name, indeces in transforms.items():
        for index, value in indeces.items():
            if name == "rotation_quaternion":
                rot[index] = value
            elif name == "location":
                loc[index] = value
            elif name == "scale":
                sca[index] = value
    return loc, rot, sca


def ConverKeyfames(tracksEle, data, convert):
    for nodeName, v0 in data.items():
        trackEle = GetChildEle(tracksEle, "Track", "nodeName", nodeName)
        keyframesEle = et.SubElement(trackEle, "KeyFrames")
        for frame in sorted(v0):
            keyframeEle = GetOrCreateChildEle(
                keyframesEle, "KeyFrame", "time",
                FloatToString(convert(frame)))
            transforms = v0[frame]
            loc, rot, sca = ConvertTransform(keyframeEle, transforms)
            if loc != VECTOR3_ZERO:
                keyframeEle.set("position", Vector3ToString(loc))
            if rot != QUATERNION_IDENTITY:
                keyframeEle.set("rotation", QuaternionToString(rot))
            if sca != VECTOR3_ONE:
                keyframeEle.set("scale", Vector3ToString(sca))


def SetKeyframeData(data, chan_name, frame, transform_name, index, value):
    data0 = data.setdefault(
        chan_name, {frame: {transform_name: {index: value}}})
    data1 = data0.setdefault(frame, {transform_name: {index: value}})
    data2 = data1.setdefault(transform_name, {index: value})
    data2.setdefault(index, value)


def ConvertCurve(tracksEle, curve, data):
    if len(curve.keyframe_points) == 0:
        return
    curve.update()
    pathType, chan_name, transform_name = GetPathData(curve.data_path)
    index = curve.array_index
    trackEle = GetChildEle(tracksEle, "Track", "nodeName", chan_name)
    channelMask = GetChannelMask(transform_name)
    if trackEle is None:
        trackEle = et.SubElement(tracksEle, "Track")
        trackEle.set("nodeName", chan_name)
    else:
        channelMask |= int(trackEle.get("channelMask"), 2)

    channelMask = bin(channelMask)
    channelMask = channelMask[channelMask.find('b') + 1:]
    trackEle.set("channelMask", channelMask)
    for ipoint, point in enumerate(curve.keyframe_points):
        frame = int(curve.keyframe_points[ipoint].co[0])
        value = curve.keyframe_points[ipoint].co[1]
        SetKeyframeData(data, chan_name, frame, transform_name, index, value)


def ConvertAnimation(animationsEle, action):
    # fps = scene.render.fps / scene.render.fps_base
    fps = 24.0
    frame_begin, frame_end = action.frame_range[0], action.frame_range[1]
    trackLength = (frame_end - frame_begin) / fps

    def ConvertFrame2Time(frame):
        return frame / fps

    animationEle = et.SubElement(animationsEle, "Animation")
    animationEle.set("length", FloatToString(trackLength))
    animationEle.set("name", action.name)
    tracksEle = et.SubElement(animationEle, "Tracks")
    data = {}
    for curve in action.fcurves:
        ConvertCurve(tracksEle, curve, data)
    ConverKeyfames(tracksEle, data, ConvertFrame2Time)


def ConvertMeshes():
    converted = []
    meshesEle = et.SubElement(appEle, "Meshes")
    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            if obj.data.name not in converted:
                ConvertMesh(obj.data.name, meshesEle, obj, 0)
                converted.append(obj.data.name)
    return meshesEle


def ConvertSkeletons():
    armaturesEle = et.SubElement(appEle, "Skeletons")
    for obj in bpy.data.objects:
        if obj.type == 'ARMATURE':
            ConvertArmature(armaturesEle, obj)
    return armaturesEle


def ConvertAnimations():
    animationsEle = et.SubElement(appEle, "Animations")
    for action in bpy.data.actions:
        ConvertAnimation(animationsEle, action)
    return animationsEle


def GetRigidBody(sceneNodeEle):
    rigidBody = sceneNodeEle.find("RigidBody")
    if rigidBody is not None:
        return sceneNodeEle, rigidBody
    parentEle = GetParentEle("SceneNode", "name", sceneNodeEle.get("name"))
    if parentEle != sceneNodeEle and parentEle is not None:
        return GetRigidBody(parentEle)
    return None, None


def GetCharacter(sceneNodeEle):
    character = sceneNodeEle.find("Character")
    if character is not None:
        return sceneNodeEle, character
    parentEle = GetParentEle("SceneNode", "name", sceneNodeEle.get("name"))
    if parentEle != sceneNodeEle and parentEle is not None:
        return GetCharacter(parentEle)
    return None, None


def HasRigidBody(obj):
    return obj and obj.game.physics_type != 'NO_COLLISION' and obj.game.physics_type != 'CHARACTER'


def HasCharacter(obj):
    return obj and obj.game.physics_type == 'CHARACTER'


def GetParentWithName(obj, name):
    if obj.parent:
        if obj.parent.name == name:
            return obj.parent
        else:
            return GetParentWithName(obj.parent, name)
    else:
        return None


def CreateRigidBody(sceneNodeEle, obj, materialIndex):
    foundParent = True
    parentSceneNodeEle, rigidBodyEle = GetRigidBody(sceneNodeEle)
    if rigidBodyEle is None:
        foundParent = False
        rigidBodyEle = et.SubElement(sceneNodeEle, "RigidBody")
        if materialIndex != -1:
            materialSlot = obj.material_slots[materialIndex]
            physics = materialSlot.material.physics
            rigidBodyEle.set("friction", FloatToString(physics.friction))
            rigidBodyEle.set("restitution", FloatToString(physics.elasticity))

        collisionGroup = 0
        for i, group in enumerate(obj.game.collision_group):
            if group:
                collisionGroup += 1 << i
        rigidBodyEle.set("collisionGroup", str(collisionGroup))

        collisionMask = 0
        for i, mask in enumerate(obj.game.collision_mask):
            if mask:
                collisionMask += 1 << i
        rigidBodyEle.set("collisionMask", str(collisionMask))

        if obj.game.physics_type == 'STATIC':
            rigidBodyEle.set("mass", "0")
        else:
            rigidBodyEle.set("mass", FloatToString(obj.game.mass))

        if obj.game.physics_type == 'SENSOR':
            rigidBodyEle.set("trigger", BoolToString(True))
        else:
            rigidBodyEle.set("trigger", BoolToString(False))

        if obj.game.physics_type == 'CHARACTER':
            rigidBodyEle.set("kinematic", BoolToString(True))
        else:
            rigidBodyEle.set("kinematic", BoolToString(False))

        rigidBodyEle.set("linearDamp", FloatToString(obj.game.damping))
        rigidBodyEle.set(
            "angularDamp", FloatToString(obj.game.rotation_damping))

        linearFactor = mathutils.Vector((1, 1, 1))
        if obj.game.lock_location_x:
            linearFactor.x = 0
        if obj.game.lock_location_y:
            linearFactor.y = 0
        if obj.game.lock_location_z:
            linearFactor.z = 0
        rigidBodyEle.set("linearFactor", Vector3ToString(linearFactor))

        angularFactor = mathutils.Vector((1, 1, 1))
        if obj.game.lock_rotation_x:
            angularFactor.x = 0
        if obj.game.lock_rotation_y:
            angularFactor.y = 0
        if obj.game.lock_rotation_z:
            angularFactor.z = 0
        rigidBodyEle.set("angularFactor", Vector3ToString(angularFactor))

    if obj.game.use_collision_bounds and obj.data:
        shapesEle = rigidBodyEle.find("Shapes")
        if shapesEle is None:
            shapesEle = et.SubElement(rigidBodyEle, "Shapes")
        shapeEle = et.SubElement(shapesEle, "Shape")
        name, scaleStr, typeStr = GetPhysicShapeName(obj, obj.data.name)
        shapeEle.set("name", name)
        if foundParent:
            parentObj = GetParentWithName(obj, parentSceneNodeEle.get("name"))
            m = parentObj.matrix_world * obj.matrix_local
            t1, r1, s1 = parentObj.matrix_world.decompose()
            t2, r2, s2 = obj.matrix_local.decompose()
            t2.x = t2.x * s1.x
            t2.y = t2.y * s1.y
            t2.z = t2.z * s1.z
            shapeEle.set("position", Vector3ToString(t2))
            shapeEle.set("orientation", QuaternionToString(r2))

            if obj.material_slots:
                materialSlot = obj.material_slots[0]
                physics = materialSlot.material.physics
                rigidBodyEle.set("friction", FloatToString(physics.friction))
                rigidBodyEle.set("restitution", FloatToString(physics.elasticity))

    return rigidBodyEle


def CreateCharacter(sceneNodeEle, obj, materialIndex):
    foundParent = True
    parentSceneNodeEle, characterEle = GetCharacter(sceneNodeEle)
    if characterEle is None:
        foundParent = False
        characterEle = et.SubElement(sceneNodeEle, "Character")
        if materialIndex != -1:
            materialSlot = obj.material_slots[materialIndex]
            physics = materialSlot.material.physics
            characterEle.set("friction", FloatToString(physics.friction))
            characterEle.set("restitution", FloatToString(physics.elasticity))

        collisionGroup = 0
        for i, group in enumerate(obj.game.collision_group):
            if group:
                collisionGroup += 1 << i
        characterEle.set("collisionGroup", str(collisionGroup))

        collisionMask = 0
        for i, mask in enumerate(obj.game.collision_mask):
            if mask:
                collisionMask += 1 << i
        characterEle.set("collisionMask", str(collisionMask))

        characterEle.set("stepHeight", FloatToString(obj.game.step_height))
        characterEle.set("jumpSpeed", FloatToString(obj.game.jump_speed))
        characterEle.set("fallSpeed", FloatToString(obj.game.fall_speed))

    if obj.game.use_collision_bounds and obj.data:
        shapesEle = characterEle.find("Shapes")
        if shapesEle is None:
            shapesEle = et.SubElement(characterEle, "Shapes")
        shapeEle = et.SubElement(shapesEle, "Shape")
        name, scaleStr, typeStr = GetPhysicShapeName(obj, obj.data.name)
        shapeEle.set("name", name)
        if foundParent:
            parentObj = GetParentWithName(obj, parentSceneNodeEle.get("name"))
            m = parentObj.matrix_world * obj.matrix_local
            t1, r1, s1 = parentObj.matrix_world.decompose()
            t2, r2, s2 = obj.matrix_local.decompose()
            t2.x = t2.x * s1.x
            t2.y = t2.y * s1.y
            t2.z = t2.z * s1.z
            shapeEle.set("position", Vector3ToString(t2))
            shapeEle.set("orientation", QuaternionToString(r2))

            if obj.material_slots:
                materialSlot = obj.material_slots[0]
                physics = materialSlot.material.physics
                characterEle.set("friction", FloatToString(physics.friction))
                characterEle.set("restitution", FloatToString(physics.elasticity))

    return characterEle


def CreatePhysics(sceneNodeEle, obj, materialIndex):
    if HasRigidBody(obj):
        CreateRigidBody(sceneNodeEle, obj, materialIndex)
    elif HasCharacter(obj):
        CreateCharacter(sceneNodeEle, obj, materialIndex)


def GetPhysicShapeName(obj, meshName):
    loc, rot, scale = obj.matrix_world.decompose()
    scaleStr = Vector3ToString(scale)
    shapeType = obj.game.collision_bounds_type
    typeStr = BoundTypeToString(shapeType)
    if shapeType == 'CONVEX_HULL' or shapeType == 'TRIANGLE_MESH':
        assert obj.type == 'MESH'
        assert len(meshName) > 0
        name = str(len(meshName)) + " " + meshName + " " + scaleStr + " " + typeStr
    else:
        name = scaleStr + " " + typeStr
    return name, scaleStr, typeStr


def MergeBoundingBox(minA, maxA, minB, maxB):
    if minA.x < minB.x:
        minB.x = minA.x
    if minA.y < minB.y:
        minB.y = minA.y
    if minA.z < minB.z:
        minB.z = minA.z
    if maxA.x > maxB.x:
        maxB.x = maxA.x
    if maxA.y > maxB.y:
        maxB.y = maxA.y
    if maxA.z > maxB.z:
        maxB.z = maxA.z
    return minB, maxB


def GetBoundingBox(obj):
    bbMin = mathutils.Vector(obj.bound_box[0])
    bbMax = mathutils.Vector(obj.bound_box[6])
    for child in obj.children:
        bbBMin, bbBMax = GetBoundingBox(child)
        bbMin, bbMax = MergeBoundingBox(bbMin, bbMax, bbBMin, bbBMax)
    return bbMin, bbMax


def CreatePhysicShape(shapesEle, obj, meshName):
    name, scaleStr, typeStr = GetPhysicShapeName(obj, meshName)
    shapeEle = GetChildEle(shapesEle, "Shape", "name", name)
    if shapeEle is None:
        shapeEle = et.SubElement(shapesEle, "Shape")
        shapeEle.set("name", name)
        shapeEle.set("scale", scaleStr)
        shapeEle.set("margin", FloatToString(obj.game.collision_margin))
        shapeEle.set("type", typeStr)
        shapeType = obj.game.collision_bounds_type
        if shapeType == 'CONVEX_HULL' or shapeType == 'TRIANGLE_MESH':
            assert obj.type == 'MESH'
            shapeEle.set("meshName", meshName)
        bbMin, bbMax = GetBoundingBox(obj)
        shapeEle.set("bb", Vector3ToString(bbMin) + Vector3ToString(bbMax))


def ConvertPhysicShape(shapesEle, obj):
    if not obj.data:
        return
    nMaterials = len(obj.material_slots)
    materialIndex = 0
    while nMaterials > materialIndex or materialIndex == 0:
        name = GetObjMaterialName(obj.data.name, obj, materialIndex)
        CreatePhysicShape(shapesEle, obj, name)
        materialIndex = materialIndex + 1


def ConvertPhysicShapes():
    shapesEle = et.SubElement(appEle, "Shapes")
    for obj in bpy.data.objects:
        if obj.game.use_collision_bounds:
            ConvertPhysicShape(shapesEle, obj)
    return shapesEle


def ConvertPhysicsScene(sceneEle, scene):
    # print("Converting physics for scene " + scene.name)
    physicsEle = et.SubElement(sceneEle, "Physics")
    gravity = scene.gravity
    gravity.z, gravity.y = gravity.y, gravity.z
    physicsEle.set("gravity", Vector3ToString(gravity))
    physicsEle.set("fps", str(scene.game_settings.fps))
    physicsEle.set("maxSubSteps", str(scene.game_settings.physics_step_max))


def ConvertScene(scene):
    print("Converting scene " + scene.name)
    sceneEle = et.SubElement(appEle, "Scene")
    sceneEle.set("name", scene.name)
    if scene.camera is not None:
        sceneEle.set("mainCamera", scene.camera.name)
    sceneEle.set("ambient", ColorToString(scene.world.ambient_color))
    sceneEle.set("horizon", ColorToString(scene.world.horizon_color))
    sceneEle.set("enableFog", BoolToString(scene.world.mist_settings.use_mist))
    sceneEle.set("fogMinIntensity", FloatToString(
        scene.world.mist_settings.intensity))
    sceneEle.set("fogStart", FloatToString(scene.world.mist_settings.start))
    sceneEle.set("fogDepth", FloatToString(scene.world.mist_settings.depth))
    sceneEle.set("fogHeight", FloatToString(scene.world.mist_settings.height))

    orientation = mathutils.Quaternion(
        (1, 0, 0), math.radians(-90.0))

    sceneNodeEle = CreateSceneNode(
        scene.name, sceneEle, None, -1, None, orientation, None)

    ConvertPhysicsScene(sceneEle, scene)

    for obj in scene.objects:
        scene.objects.active = obj
        currentMode = obj.mode
        if currentMode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
        parentEle = sceneNodeEle
        if obj.parent:
            parentEle = GetChildEle(sceneNodeEle, "SceneNode", "name", obj.parent.name)
            if parentEle is None:
                print("Warning parent=" + obj.parent.name + " for " + obj.name + " NOT FOUND!!!")
                continue
        ConvertObject(parentEle, obj)
        if currentMode != 'OBJECT':
            bpy.ops.object.mode_set(mode=currentMode)


def ConvertScenes():
    for scene in bpy.data.objects.data.scenes:
        ConvertScene(scene)


def ConvertApp():
    if not basedir:
        raise Exception("Blend file is not saved")
    filename = os.path.splitext(
        os.path.basename(bpy.data.filepath))[0]
    defaultMaterial = bpy.data.materials.new(DEFAULT_MATERIAL_NAME)
    ConvertResources(False)
    ConvertMaterials()
    ConvertMeshes()
    ConvertSkeletons()
    ConvertAnimations()
    ConvertPhysicShapes()
    ConvertScenes()
    bpy.data.materials.remove(defaultMaterial)
    tree = et.ElementTree(appEle)
    tree.write(datadir + "/" + filename + ".xml")

appEle = et.Element("App")
basedir = os.path.dirname(bpy.data.filepath)

argv = sys.argv
argv = argv[argv.index("--") + 1:]  # get all args after "--"
if len(argv) > 0:
    datadir = argv[0]
else:
    datadir = basedir + "/../data"

print("Exporting to directory " + datadir)
ConvertApp()
