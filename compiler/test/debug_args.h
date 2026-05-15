// //#pragma file: compiler/test/debug_args.h
// #include "libgreenc.h"
// #include "lang/lang.h"
// #include "args.h"


// int debug_args(args_result args) {
 
// 	print("debug: positional count");
// 	buf32 s = tostring(args.positional.count);
// 	print(s.str);
	
// 	print("debug: options count");
// 	print(str(args.options.count));
	
// 	print("debug: option_values count");
// 	print(str(args.option_values.count));
	
// 	for (int i = 0; i < args.options.count; i++) {
	
// 	    print("option:");
// 	    print(args.options.items[i]);
	
// 	    print("value:");
	
// 	    if (args.option_values.items[i]) {
// 	        print(args.option_values.items[i]);
// 	    }
	
// 	    else {
// 	        print("(null)");
// 	    }
// 	}
	
// 	for (int i = 0; i < args.positional.count; i++) {
	
// 	    print("positional:");
// 	    print(args.positional.items[i]);
// 	}

// }
