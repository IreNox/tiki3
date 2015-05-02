
#include "tiki/editornative/editorsystem.hpp"

#include "tiki/graphics/graphicssystem.hpp"
#include "tiki/graphics/immediaterenderer.hpp"
#include "tiki/resource/resourcemanager.hpp"
#include "tiki/io/gamebuildfilesystem.hpp"
#include "tiki/runtimeshared/frameworkfactories.hpp"

#include "tiki/editornative/transformgizmo.hpp"

namespace tiki
{
	struct EditorData
	{
		GraphicsSystem		graphicsSystem;
		ImmediateRenderer	immediateRenderer;
		ResourceManager		resourceManager;
		GamebuildFileSystem fileSystem;
		FrameworkFactories	frameworkFactories;
	};


	EditorSystem::EditorSystem()
	{

	}

	bool EditorSystem::create( EditorParameters^ parameters )
	{
		m_pData = new EditorData();

		m_TransformGizmo = gcnew TransformGizmo();

		GraphicsSystemParameters graphicsParams;
		graphicsParams.pWindowHandle = (void*)parameters->windowHandle;
		if ( !m_pData->graphicsSystem.create( graphicsParams ) )
		{
			dispose();
			return false;
		}

		m_pData->fileSystem.create( "../../../../../../gamebuild/");

		ResourceManagerParameters resourceParams;
		resourceParams.pFileSystem = &m_pData->fileSystem;

		if ( !m_pData->resourceManager.create(resourceParams) )
		{
			dispose();
			return false;
		}

		m_pData->frameworkFactories.create( m_pData->resourceManager, m_pData->graphicsSystem );


		if ( !m_pData->immediateRenderer.create( m_pData->graphicsSystem, m_pData->resourceManager ) )
		{
			dispose();
			return false;
		}

		return true;
	}

	void EditorSystem::dispose()
	{
		if ( m_pData == nullptr )
		{
			return;
		}

		m_pData->immediateRenderer.dispose( m_pData->graphicsSystem, m_pData->resourceManager );
		m_pData->frameworkFactories.dispose(m_pData->resourceManager);
		m_pData->resourceManager.dispose();
		m_pData->graphicsSystem.dispose();
		m_pData->fileSystem.dispose();

		delete m_pData;
		m_pData = nullptr;
	}

	void EditorSystem::update()
	{
		
	}

	void EditorSystem::render()
	{
		GraphicsContext& context = m_pData->graphicsSystem.beginFrame();

		
		context.clear(context.getBackBuffer(), TIKI_COLOR_BLUE);



		m_pData->graphicsSystem.endFrame();
	}

	void EditorSystem::onKeyDown( KeyEventArgs^ e )
	{

	}

	void EditorSystem::onKeyUp( KeyEventArgs^ e )
	{

	}

	void EditorSystem::onMouseDown( MouseButtonEventArgs^ e )
	{

	}

	void EditorSystem::onMouseUp( MouseButtonEventArgs^ e )
	{

	}

	void EditorSystem::onMouseDoubleClick( MouseButtonEventArgs^ e )
	{

	}

	void EditorSystem::onMouseMove( MouseEventArgs^ e )
	{

	}

}