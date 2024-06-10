// Copyright (c) ZeroC, Inc.
#import "Properties.h"

#import "Convert.h"

@implementation ICEProperties

- (std::shared_ptr<Ice::Properties>)properties
{
    return std::static_pointer_cast<Ice::Properties>(self.cppObject);
}

- (NSString*)getProperty:(NSString*)key
{
    return toNSString(self.properties->getProperty(fromNSString(key)));
}

- (NSString*)getIceProperty:(NSString*)key
{
    try
    {
        toNSString(self.properties->getIceProperty(fromNSString(key)));
    }
    catch (const std::invalid_argument&)
    {
        return nil;
    }
    return toNSString(self.properties->getIceProperty(fromNSString(key)));
}

- (NSString*)getPropertyWithDefault:(NSString*)key value:(NSString*)value
{
    return toNSString(self.properties->getPropertyWithDefault(fromNSString(key), fromNSString(value)));
}

- (int32_t)getPropertyAsInt:(NSString*)key
{
    return self.properties->getPropertyAsInt(fromNSString(key));
}

- (id)getIcePropertyAsInt:(NSString*)key
{
    try
    {
        int32_t value = self.properties->getIcePropertyAsInt(fromNSString(key));
        return [NSNumber numberWithInt:value];
    }
    catch (const std::invalid_argument&)
    {
        return nil;
    }
}

- (int32_t)getPropertyAsIntWithDefault:(NSString*)key value:(int32_t)value
{
    return self.properties->getPropertyAsIntWithDefault(fromNSString(key), value);
}

- (NSArray<NSString*>*)getPropertyAsList:(NSString*)key
{
    return toNSArray(self.properties->getPropertyAsList(fromNSString(key)));
}

- (NSArray<NSString*>*)getIcePropertyAsList:(NSString*)key
{
    return toNSArray(self.properties->getIcePropertyAsList(fromNSString(key)));
}

- (NSArray<NSString*>*)getPropertyAsListWithDefault:(NSString*)key value:(NSArray<NSString*>*)value
{
    std::vector<std::string> s;
    fromNSArray(value, s);
    return toNSArray(self.properties->getPropertyAsListWithDefault(fromNSString(key), s));
}

- (NSDictionary<NSString*, NSString*>*)getPropertiesForPrefix:(NSString*)prefix
{
    return toNSDictionary(self.properties->getPropertiesForPrefix(fromNSString(prefix)));
}

- (BOOL)setProperty:(NSString*)key value:(NSString*)value error:(NSError**)error;
{
    try
    {
        self.properties->setProperty(fromNSString(key), fromNSString(value));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (NSArray<NSString*>*)getCommandLineOptions
{
    return toNSArray(self.properties->getCommandLineOptions());
}

- (NSArray<NSString*>*)parseCommandLineOptions:(NSString*)prefix
                                       options:(NSArray<NSString*>*)options
                                         error:(NSError**)error;
{
    try
    {
        std::vector<std::string> s;
        fromNSArray(options, s);
        return toNSArray(self.properties->parseCommandLineOptions(fromNSString(prefix), s));
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (NSArray<NSString*>*)parseIceCommandLineOptions:(NSArray<NSString*>*)options error:(NSError**)error;
{
    try
    {
        std::vector<std::string> s;
        fromNSArray(options, s);
        return toNSArray(self.properties->parseIceCommandLineOptions(s));
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return nil;
    }
}

- (BOOL)load:(NSString*)file error:(NSError**)error
{
    try
    {
        self.properties->load(fromNSString(file));
        return YES;
    }
    catch (...)
    {
        *error = convertException(std::current_exception());
        return NO;
    }
}

- (ICEProperties*)clone
{
    auto props = self.properties->clone();
    return [ICEProperties getHandle:props];
}

@end
