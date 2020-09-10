#import "UIColor+HBAdditions.h"

@implementation UIColor (HBColorPickerAdditions)

- (instancetype)initWithHbcp_propertyListValue:(id)value {
	if (!value) {
		return nil;
	} else if ([value isKindOfClass:NSArray.class] && (((NSArray *)value).count == 3 || ((NSArray *)value).count == 4)) {
		NSArray *array = value;
		return [self initWithRed:((NSNumber *)array[0]).integerValue / 255.f
		                   green:((NSNumber *)array[1]).integerValue / 255.f
		                    blue:((NSNumber *)array[2]).integerValue / 255.f
		                    alpha:array.count == 4 ? ((NSNumber *)array[3]).doubleValue : 1];
	} else if ([value isKindOfClass:NSString.class]) {
		NSString *string = value;
		NSUInteger colonLocation = [string rangeOfString:@":"].location;
		NSString *alphaString = nil;
		if (colonLocation != NSNotFound) {
			alphaString = [string substringFromIndex:colonLocation + 1];
			string = [string substringToIndex:colonLocation];
		}

		if (string.length == 4 || string.length == 5) {
			NSString *r = [string substringWithRange:NSMakeRange(1, 1)];
			NSString *g = [string substringWithRange:NSMakeRange(2, 1)];
			NSString *b = [string substringWithRange:NSMakeRange(3, 1)];
			NSString *a = string.length == 5 ? [string substringWithRange:NSMakeRange(4, 1)] : @"F";
			string = [NSString stringWithFormat:@"#%1$@%1$@%2$@%2$@%3$@%3$@%4$@%4$@", r, g, b, a];
		}

		unsigned int hex = 0;
		NSScanner *scanner = [NSScanner scannerWithString:string];
		scanner.charactersToBeSkipped = [NSCharacterSet characterSetWithCharactersInString:@"#"];
		[scanner scanHexInt:&hex];

		if (string.length == 9) {
			return [self initWithRed:((hex & 0xFF000000) >> 24) / 255.f
			                   green:((hex & 0x00FF0000) >> 16) / 255.f
			                    blue:((hex & 0x0000FF00) >> 8)  / 255.f
			                   alpha:((hex & 0x000000FF) >> 0)  / 255.f];
		} else {
			CGFloat alpha = 1;
			if (alphaString.length > 0) {
				NSScanner *alphaScanner = [NSScanner scannerWithString:alphaString];
				if (![alphaScanner scanDouble:&alpha]) {
					alpha = 1;
				}
			}
			return [self initWithRed:((hex & 0xFF0000) >> 16) / 255.f
			                   green:((hex & 0x00FF00) >> 8)  / 255.f
			                    blue:((hex & 0x0000FF) >> 0)  / 255.f
			                   alpha:alpha];
		}
	}

	return nil;
}

- (NSString *)hbcp_propertyListValue {
	CGFloat r, g, b, a;
	[self getRed:&r green:&g blue:&b alpha:&a];
	unsigned int hex =
		(((unsigned int)(r * 255.0) & 0xFF) << 16) +
		(((unsigned int)(g * 255.0) & 0xFF) << 8) +
		((unsigned int)(b * 255.0) & 0xFF);
	NSString *alphaString = a == 1 ? @"" : [NSString stringWithFormat:@":%.5G", a];
	return [NSString stringWithFormat:@"#%06X%@", hex, alphaString];
}

@end
