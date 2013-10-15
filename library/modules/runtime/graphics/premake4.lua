-- library/modules/runtime/graphics

local module = Module:new( "graphics" );

module:add_files( "source/*.*" );
module:add_files( "include/**/*.hpp" );
module:add_include_dir( "include" );
module:add_include_dir( "source" );

module:add_dependency( "math" );
module:add_dependency( "graphicsbase" );
--module:add_dependency( "graphicscomponents" );
module:add_dependency( "graphicsresources" );
