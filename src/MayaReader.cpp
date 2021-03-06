#include "MayaReader.h"

#define _BOOL bool
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>

#include <maya/MArgList.h>
#include <maya/MFileObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <maya/MSimple.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MLibrary.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnPhongShader.h>

#include "Model.h"
#include "VertexDefinition.h"
#include "SubMesh.h"
#include "Material.h"
#include "Vector3MaterialParameter.h"
#include "Vector4MaterialParameter.h"
#include "FloatMaterialParameter.h"

MIntArray GetLocalIndex( MIntArray & getVertices, MIntArray & getTriangle )
{
	MIntArray   localIndex;
	unsigned    gv, gt;

	assert ( getTriangle.length() == 3 );    // Should always deal with a triangle

	for ( gt = 0; gt < getTriangle.length(); gt++ )
	{
		for ( gv = 0; gv < getVertices.length(); gv++ )
		{
			if ( getTriangle[gt] == getVertices[gv] )
			{
				localIndex.append( gv );
				break;
			}
		}

		// if nothing was added, add default "no match"
		if ( localIndex.length() == gt )
			localIndex.append( -1 );
	}

	return localIndex;
}

inline float clamp(float x, float a, float b)
{
	return x < a ? a : (x > b ? b : x);
}

std::string extractAssetPath(const std::string fullAssetPath) {
	std::string path = fullAssetPath;
	std::string filename;

	std::string assets("assets/");
	size_t pos = path.rfind(assets);
	if(pos != std::string::npos)
		filename.assign(path.begin() + pos + assets.length(), path.end());
	else
		filename = path;

	return filename;
}


void printMaterials(const MDagPath& dagPath) {
	MFnMesh fnMesh(dagPath);
	unsigned instanceNumber = dagPath.instanceNumber();
	MObjectArray sets;
	MObjectArray comps;
	fnMesh.getConnectedSetsAndMembers( instanceNumber, sets, comps, true );

	// Print Material Names
	for(unsigned int i = 0; i < sets.length(); ++i)
	{
		MFnDependencyNode fnDepSGNode(sets[i]);
		std::cout << fnDepSGNode.name() << std::endl;
	}
}

void extractPolygons(Model* model) {

	MStatus stat;
	MItDag dagIter(MItDag::kBreadthFirst, MFn::kInvalid);

	for (; !dagIter.isDone(); dagIter.next()) {
		MDagPath dagPath;
		stat = dagIter.getPath(dagPath);
		if (!stat) { continue; };

		MFnDagNode dagNode(dagPath, &stat);

		if (dagNode.isIntermediateObject()) continue;
		if (!dagPath.hasFn( MFn::kMesh )) continue;
		if (dagPath.hasFn( MFn::kTransform)) continue;
		if (!dagPath.isVisible()) continue;

		MFnMesh fnMesh(dagPath);

		MStringArray  UVSets;
		stat = fnMesh.getUVSetNames( UVSets );

		// Get all UVs for the first UV set.
		MFloatArray   u, v;
		fnMesh.getUVs(u, v, &UVSets[0]);


		MPointArray vertexList;
		fnMesh.getPoints(vertexList, MSpace::kObject);

		MFloatVectorArray  meshNormals;
		fnMesh.getNormals(meshNormals);


		unsigned instanceNumber = dagPath.instanceNumber();
		MObjectArray sets;
		MObjectArray comps;
		fnMesh.getConnectedSetsAndMembers( instanceNumber, sets, comps, true );

		//printMaterials(dagPath);

		unsigned int comlength = comps.length();

		for (unsigned int compi = 0; compi < comlength; compi++) {

			SubMesh submesh;

			MItMeshPolygon itPolygon(dagPath, comps[compi]);

			unsigned int polyCount = 0;
			for (; !itPolygon.isDone(); itPolygon.next()) {
				polyCount++;
				MIntArray                           polygonVertices;
				itPolygon.getVertices(polygonVertices);

				int count;
				itPolygon.numTriangles(count);

				for (; count > -1; count--) {
					MPointArray                     nonTweaked;
					MIntArray                       triangleVertices;
					MIntArray                       localIndex;

					MStatus  status;
					status = itPolygon.getTriangle(count, nonTweaked, triangleVertices, MSpace::kObject);

					if (status == MS::kSuccess) {

						VertexDefinition vertex1;
						VertexDefinition vertex2;
						VertexDefinition vertex3;

						{ // vertices

							int vertexCount = vertexList.length();

							{
								int vertexIndex0 = triangleVertices[0];
								MPoint point0 = vertexList[vertexIndex0];

								vertex1.vertex.x = (float)point0.x;
								vertex1.vertex.y = (float)point0.y;
								vertex1.vertex.z = (float)point0.z;
							}

							{
								int vertexIndex0 = triangleVertices[1];
								MPoint point0 = vertexList[vertexIndex0];

								vertex2.vertex.x = (float)point0.x;
								vertex2.vertex.y = (float)point0.y;
								vertex2.vertex.z = (float)point0.z;
							}

							{
								int vertexIndex0 = triangleVertices[2];
								MPoint point0 = vertexList[vertexIndex0];

								vertex3.vertex.x = (float)point0.x;
								vertex3.vertex.y = (float)point0.y;
								vertex3.vertex.z = (float)point0.z;
							}

						}

						{ // normals

							// Get face-relative vertex indices for this triangle
							localIndex = GetLocalIndex(polygonVertices, triangleVertices);

							{
								int index0 = itPolygon.normalIndex(localIndex[0]);
								MPoint point0 = meshNormals[index0];

								vertex1.normal.x = (float)point0.x;
								vertex1.normal.y = (float)point0.y;
								vertex1.normal.z = (float)point0.z;
							}

							{
								int index0 = itPolygon.normalIndex(localIndex[1]);
								MPoint point0 = meshNormals[index0];

								vertex2.normal.x = (float)point0.x;
								vertex2.normal.y = (float)point0.y;
								vertex2.normal.z = (float)point0.z;
							}

							{
								int index0 = itPolygon.normalIndex(localIndex[2]);
								MPoint point0 = meshNormals[index0];

								vertex3.normal.x = (float)point0.x;
								vertex3.normal.y = (float)point0.y;
								vertex3.normal.z = (float)point0.z;
							}
						}

						{ // uvs

							int uvID[3];

							MStatus uvFetchStatus;
							for (unsigned int vtxInPolygon = 0; vtxInPolygon < 3; vtxInPolygon++) {
								uvFetchStatus = itPolygon.getUVIndex(localIndex[vtxInPolygon], uvID[vtxInPolygon]);
							}

							if (uvFetchStatus == MStatus::kSuccess) {

								{
									int index0 = uvID[0];
									float uvu = u[index0];
									float uvv = v[index0];

									vertex1.uv.x = uvu;
									vertex1.uv.y = 1.0f - uvv;
								}

								{
									int index0 = uvID[1];
									float uvu = u[index0];
									float uvv = v[index0];

									vertex2.uv.x = uvu;
									vertex2.uv.y = 1.0f - uvv;
								}

								{
									int index0 = uvID[2];
									float uvu = u[index0];
									float uvv = v[index0];

									vertex3.uv.x = uvu;
									vertex3.uv.y = 1.0f - uvv; // directx
								}
							}
						}

						submesh.addVertex(vertex1);
						submesh.addVertex(vertex2);
						submesh.addVertex(vertex3);
					}
				}
			}

			{
				Material material;

				{
					MObjectArray shaders;
					MIntArray indices;
					fnMesh.getConnectedShaders(0, shaders, indices);
					unsigned int shaderCount = shaders.length();

					MPlugArray connections;
					MFnDependencyNode shaderGroup(shaders[compi]);
					MPlug shaderPlug = shaderGroup.findPlug("surfaceShader");
					shaderPlug.connectedTo(connections, true, false);

					for(unsigned int u = 0; u < connections.length(); u++) {
						if(connections[u].node().hasFn(MFn::kLambert)) {

							MFnLambertShader lambertShader(connections[u].node());
							MPlugArray plugs;
							lambertShader.findPlug("color").connectedTo(plugs, true, false);

							for (unsigned int p = 0; p < plugs.length(); p++) {
								MPlug object = plugs[p];
								if (!object.isNull()) {
									MObject node = object.node();
									if (node.hasFn(MFn::kFileTexture)) {
										MFnDependencyNode* diffuseMapNode = new MFnDependencyNode(node);

										MPlug filenamePlug = diffuseMapNode->findPlug ("fileTextureName");
										MString mayaFileName;
										filenamePlug.getValue (mayaFileName);
										std::string diffuseMapFileName = mayaFileName.asChar();
										std::string diffuseMapAssetPath = extractAssetPath(diffuseMapFileName);
										material.addTexture("ColorMap", diffuseMapAssetPath);
									}
								} 
							}

							MColor color = lambertShader.color();
							MString materialNameRaw = lambertShader.name();
							std::string materialName = materialNameRaw.asChar();
							material.setName(materialName);

							Vector4MaterialParameter* diffuseColorParameter = new Vector4MaterialParameter("DiffuseColor");
							diffuseColorParameter->value.x = color.r;
							diffuseColorParameter->value.y = color.g;
							diffuseColorParameter->value.z = color.b;
							diffuseColorParameter->value.w = color.a;

							material.addParameter(diffuseColorParameter);
						}
					}
				}

				{
					if (material.hasTextures()) {
						material.setEffect("shaders/deferred_render_colormap_normal_depth.cg");
					}
					else {
						material.setEffect("shaders/deferred_render_color_normal_depth.cg");
					}
				}


				{
					FloatMaterialParameter* specularPowerParameter = new FloatMaterialParameter("SpecularPower");
					specularPowerParameter->value = 1.0;
					material.addParameter(specularPowerParameter);
				}

				{
					FloatMaterialParameter* specularIntensityParameter = new FloatMaterialParameter("SpecularIntensity");
					specularIntensityParameter->value = 1.0f;
					material.addParameter(specularIntensityParameter);
				}

				{
					FloatMaterialParameter* diffusePowerParameter = new FloatMaterialParameter("DiffusePower");
					diffusePowerParameter->value = 1.0f;
					material.addParameter(diffusePowerParameter);
				}

				submesh.setMaterial(material);
			}

			model->addSubMesh(submesh);
		}
	}
}

Model* MayaReader::read(const char* filename) {

	MStatus status = MLibrary::initialize(filename);
	if (!status) {
		std::cerr << "Failed to initialize Maya" << std::endl;
		return NULL;
	}

	MFileIO::newFile(true);

	status = MFileIO::open(filename);
	if ( !status ) {
		std::cerr << "Failed to open Maya source file: " << status.errorString().asUTF8() << std::endl;
		return NULL;
	}

	status = MGlobal::executeCommand( "delete -ch" );
	if (!status) {
		std::cerr << "Failed to cleanup maya source objects" << std::endl;
		return NULL;
	}

	Model* model = new Model();
	//extractLayers(model);
	extractPolygons(model);

	//MLibrary::cleanup();

	//extractGeometry(model);

	return model;
}

bool MayaReader::acceptExtension(const std::string& extension) {
	bool isMayaBinary = extension.compare("mb") == 0;
	bool isMayaAscii = extension.compare("ma") == 0;
	return isMayaBinary || isMayaAscii;
}
