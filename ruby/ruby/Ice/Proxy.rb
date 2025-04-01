# Copyright (c) ZeroC, Inc.

module Ice

    T_ObjectPrx = Ice.__declareProxy('::Ice::Object')
    T_ObjectPrx.defineProxy(ObjectPrx, nil, [])

    # Provide some common functionality for proxy classes
    module Proxy_mixin
        module ClassMethods
            def ice_staticId()
                self::ICE_ID
            end

            def checkedCast(proxy, facetOrContext=nil, context=nil)
                ice_checkedCast(proxy, self::ICE_ID, facetOrContext, context)
            end
        end

      def self.included(base)
          base.extend(ClassMethods)
      end
    end

    module ObjectPrx_mixin
        def ice_ping(context=nil)
            ObjectPrx_mixin::OP_ice_ping.invoke(self, [], context)
        end

        def ice_isA(id, context=nil)
            return ObjectPrx_mixin::OP_ice_isA.invoke(self, [id], context)
        end

        def ice_ids(context=nil)
            ObjectPrx_mixin::OP_ice_ids.invoke(self, [], context)
        end

        def ice_id(context=nil)
            ObjectPrx_mixin::OP_ice_id.invoke(self, [], context)
        end
    end

    ObjectPrx_mixin::OP_ice_ping = Ice::__defineOperation('ice_ping', Ice::OperationMode::Idempotent, nil, [[::Ice::T_Identity, false, 0]], [], [Ice::T_ObjectPrx, false, 0], [::Ice::T_ObjectNotFoundException])

    class ObjectPrx
        include Proxy_mixin
        include ObjectPrx_mixin
    end

    # Proxy comparison functions.

    def Ice.proxyIdentityCompare(lhs, rhs)
      if (lhs && !lhs.is_a?(ObjectPrx)) || (rhs && !rhs.is_a?(ObjectPrx))
          raise TypeError, 'argument is not a proxy'
      end
      if lhs.nil? && rhs.nil?
          return 0
      elsif lhs.nil? && rhs
          return -1
      elsif lhs && rhs.nil?
          return 1
      else
          return lhs.ice_getIdentity() <=> rhs.ice_getIdentity()
      end
    end

    def Ice.proxyIdentityEqual(lhs, rhs)
        return proxyIdentityCompare(lhs, rhs) == 0
    end

    def Ice.proxyIdentityAndFacetCompare(lhs, rhs)
        n = proxyIdentityCompare(lhs, rhs)
        if n == 0 && lhs && rhs
            n = lhs.ice_getFacet() <=> rhs.ice_getFacet()
        end
        return n
    end

    def Ice.proxyIdentityAndFacetEqual(lhs, rhs)
        return proxyIdentityAndFacetCompare(lhs, rhs) == 0
    end
end
