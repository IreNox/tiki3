-- library/thirdparty/lua

local module = Module:new( "lua" );

module:add_files( "lua.lua" );
module:set_base_path( "blobs/lua" );

module:add_files( "*.h" );
module:add_files( "*.c" );
module:add_files( "lua.c", {exclude = true} );
module:add_files( "luac.c", {exclude = true} );

module:add_include_dir( "src" );
