-- library/modules/tool/editorbase

local module = Module:new( "editorbase" );

module:add_files( "source/*.*" );
module:add_files( "include/**/*.hpp" );
module:add_include_dir( "include" );

module:add_dependency( "base" );