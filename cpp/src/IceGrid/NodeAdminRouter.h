// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_GRID_NODE_ADMIN_ROUTER_H
#define ICE_GRID_NODE_ADMIN_ROUTER_H

#include <IceGrid/NodeI.h>
#include <IceGrid/AdminRouter.h>

namespace IceGrid
{
    //
    // Routes requests to a server's admin object through the Node
    //
    class NodeServerAdminRouter : public AdminRouter
    {
    public:
        NodeServerAdminRouter(const NodeIPtr&);

        virtual void ice_invoke_async(const Ice::AMD_Object_ice_invokePtr&,
                                      const std::pair<const Ice::Byte*, const Ice::Byte*>&, const Ice::Current&);

    private:
        NodeIPtr _node;
    };

} // namespace IceGrid
#endif
