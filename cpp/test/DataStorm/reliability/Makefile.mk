#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

$(test)_programs        = reader writer
$(test)_dependencies    = DataStorm Ice TestCommon

$(test)_reader_sources       = Reader.cpp
$(test)_writer_sources      = Writer.cpp

tests += $(test)
