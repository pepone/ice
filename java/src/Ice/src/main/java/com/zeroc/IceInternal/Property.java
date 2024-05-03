//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceInternal;

public class Property {
  public Property(String pattern, String defaultValue, boolean deprecated, String deprecatedBy) {
    _pattern = pattern;
    _defaultValue = defaultValue;
    _deprecated = deprecated;
    _deprecatedBy = deprecatedBy;
  }

  public String pattern() {
    return _pattern;
  }

  public String defaultValue() {
    return _defaultValue;
  }

  public boolean deprecated() {
    return _deprecated;
  }

  public String deprecatedBy() {
    return _deprecatedBy;
  }

  private String _pattern;
  private String _defaultValue;
  private boolean _deprecated;
  private String _deprecatedBy;
}
