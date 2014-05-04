
#include "main.h"


void AppTest::init(ml::ApplicationData &app)
{

    //const std::string testPLY = "scans/gates381.ply";
    //const TriMesh triMesh = OpenMeshLoader::load(testPLY);
    //m_mesh.load(app.graphics, triMesh);

	//const std::string testFilename = "scans/gates381.off";
	//const std::string testFilename = "scans/gates381.obj";
	const std::string testFilename = "scans/gates381_full.ply";
	//const std::string testFilename = "scans/gates381_ascii.ply";
	ml::MeshDataf meshData = ml::MeshIOf::loadFromFile(testFilename);
	//ml::MeshIOf::writeToFile("outtest.ply", meshData);
	//ml::MeshIOf::writeToFile("outtest.off", meshData);
	//ml::MeshIOf::writeToFile("outtest.obj", meshData);

	ml::TriMeshf triMesh(meshData);
	//ml::TriangleBVHAcceleratorf accel(triMesh.getTrianglePointers());
	//ml::Rayf ray(ml::vec3f::origin, ml::vec3f::origin);
	//float u,v,t; 
	//ml::Trianglef* tri;
	//accel.intersect(ray, u, v, t, tri);


	//ml::MeshDataf out = triMesh.getMeshData();
	//ml::MeshIOf::writeToFile("bla.ply", out);

	//const ml::TriMesh triMesh(meshData);
	m_mesh.load(app.graphics, meshData);
	//std::vector<ml::vec4f> color(meshData.m_Vertices.size(), ml::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	//m_mesh.updateAttributeA(app.graphics, color);

	auto lambdaPoints = [=] (ml::vec3f& v) { v = ml::vec3f(-2.f*(float)rand() / RAND_MAX, -2.f*(float)rand() / RAND_MAX, (float)rand() / RAND_MAX);};
	std::vector<ml::vec3f> points(5000);
	std::for_each(points.begin(), points.end(), lambdaPoints);

    m_pointCloud.load(app.graphics, ml::meshutil::createPointCloudTemplate(ml::shapes::box(0.01f), points));

    m_vsColor.load(app.graphics, "shaders/test.shader");
    m_psColor.load(app.graphics, "shaders/test.shader");

    m_vsPointCloud.load(app.graphics, "shaders/pointCloud.shader");
    m_psPointCloud.load(app.graphics, "shaders/pointCloud.shader");

    m_constants.init(app.graphics);

    //vec3f eye(1.0f, 2.0f, 3.0f);
    ml::vec3f eye(-0.5f, -0.5f, 1.5f);
    ml::vec3f worldUp(0.0f, 0.0f, 1.0f);
    m_camera = ml::Camera(eye, worldUp, ml::vec3f::eX, 60.0f, (float)app.window.width() / app.window.height(), 0.01f, 1000.0f);

    m_font.init(app.graphics, "Calibri");

}

void AppTest::render(ml::ApplicationData &app)
{
    m_timer.frame();

    ConstantBuffer constants;
    constants.worldViewProj = m_camera.cameraPerspective();
    m_constants.update(app.graphics, constants);

    m_vsColor.bind(app.graphics);
    m_psColor.bind(app.graphics);
    m_constants.bindVertexShader(app.graphics, 0);

    m_mesh.render(app.graphics);

    m_vsPointCloud.bind(app.graphics);
    m_psPointCloud.bind(app.graphics);
    m_constants.bindVertexShader(app.graphics, 0);

    m_pointCloud.render(app.graphics);

    m_font.drawString(app.graphics, "FPS: " + ml::convert::toString(m_timer.framesPerSecond()), ml::vec2i(10, 5), 24.0f, ml::RGBColor::Red);
}

void AppTest::resize(ml::ApplicationData &app)
{
    m_camera.updateAspectRatio((float)app.window.width() / app.window.height());
}

void AppTest::keyDown(ml::ApplicationData &app, UINT key)
{

}

void AppTest::keyPressed(ml::ApplicationData &app, UINT key)
{
    const float distance = 0.1f;
    const float theta = 5.0f;

    if(key == KEY_S) m_camera.move(-distance);
    if(key == KEY_W) m_camera.move(distance);
    if(key == KEY_A) m_camera.strafe(-distance);
    if(key == KEY_D) m_camera.strafe(distance);
	if(key == KEY_E) m_camera.jump(distance);
	if(key == KEY_Q) m_camera.jump(-distance);

    if(key == KEY_UP) m_camera.lookUp(theta);
    if(key == KEY_DOWN) m_camera.lookUp(-theta);
    if(key == KEY_LEFT) m_camera.lookRight(theta);
    if(key == KEY_RIGHT) m_camera.lookRight(-theta);

	if(key == 'R') {
		const std::string testFilename = "scans/gates381_full.ply";
		ml::MeshDataf meshData = ml::MeshIOf::loadFromFile(testFilename);
		ml::TriMeshf triMesh(meshData);

		std::cout << triMesh.getBoundingBox() << std::endl;

		ml::Timer c0;
		c0.start();
		ml::TriangleBVHAcceleratorf accel(triMesh);
		std::cout << "time construct " << c0.getElapsedTimeMS() << std::endl;

		ml::mat4f projToCam = m_camera.perspective().getInverse();
		ml::mat4f camToWorld = m_camera.camera().getInverse();
		ml::mat4f trans =  camToWorld * projToCam;
		ml::ColorImageRGB image(app.window.height(), app.window.width());

		ml::Timer c;
		c.start();
#pragma omp parallel for
		for (int i_ = 0; i_ < (int)app.window.height(); i_++) {
			unsigned int i = (unsigned int)i_;
			for (unsigned int j = 0; j < app.window.width(); j++) {
				//std::cout << " tyring ray " << i << " " << j << std::endl;
				ml::vec4f p((float)j, (float)i, 0.5f, 1.0f);
				p.x /= app.window.width();
				p.y /= app.window.height();
				p.x = 2.0f*p.x - 1.0f;
				p.y = 1.0f-2.0f*p.y;

				p = trans * p;
				p /= p.w;
				ml::Rayf r(m_camera.getEye(), (ml::vec3f(p.x,p.y,p.z)-m_camera.getEye()).getNormalized());
				float t,u,v;	ml::TriMeshf::Trianglef* tri;
				if (accel.intersect(r, t, u, v, tri)) {
					image(i,j) = tri->getSurfaceColor(u,v);
				} else {
					image(i,j) = 0;
				}
			
			}
		}
		std::cout << "time trace " << c.getElapsedTimeMS() << std::endl;
		ml::FreeImageWrapper::saveImage("test.jpg", image);

	}
}

void AppTest::mouseDown(ml::ApplicationData &app, ml::MouseButtonType button)
{

}

void AppTest::mouseWheel(ml::ApplicationData &app, int wheelDelta)
{
    const float distance = 0.002f;
    m_camera.move(distance * wheelDelta);
}

void AppTest::mouseMove(ml::ApplicationData &app)
{
    const float distance = 0.01f;
    const float theta = 0.5f;

    ml::vec2i posDelta = app.input.mouse.pos - app.input.prevMouse.pos;

    if(app.input.mouse.buttons[ml::MouseButtonRight])
    {
        m_camera.strafe(-distance * posDelta.x);
        m_camera.jump(distance * posDelta.y);
    }

    if(app.input.mouse.buttons[ml::MouseButtonLeft])
    {
        m_camera.lookRight(-theta * posDelta.x);
        m_camera.lookUp(theta * posDelta.y);
    }

}
