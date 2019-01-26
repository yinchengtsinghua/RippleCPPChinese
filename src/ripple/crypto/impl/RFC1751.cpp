
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2013 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#include <ripple/crypto/RFC1751.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/copied.hpp>
#include <cassert>
#include <cstdint>
#include <string>

namespace ripple {

//
//RFC 1751代码转换为C++Boost。
//

char const* RFC1751::s_dictionary [2048] =
{
    "A", "ABE", "ACE", "ACT", "AD", "ADA", "ADD",
    "AGO", "AID", "AIM", "AIR", "ALL", "ALP", "AM", "AMY", "AN", "ANA",
    "AND", "ANN", "ANT", "ANY", "APE", "APS", "APT", "ARC", "ARE", "ARK",
    "ARM", "ART", "AS", "ASH", "ASK", "AT", "ATE", "AUG", "AUK", "AVE",
    "AWE", "AWK", "AWL", "AWN", "AX", "AYE", "BAD", "BAG", "BAH", "BAM",
    "BAN", "BAR", "BAT", "BAY", "BE", "BED", "BEE", "BEG", "BEN", "BET",
    "BEY", "BIB", "BID", "BIG", "BIN", "BIT", "BOB", "BOG", "BON", "BOO",
    "BOP", "BOW", "BOY", "BUB", "BUD", "BUG", "BUM", "BUN", "BUS", "BUT",
    "BUY", "BY", "BYE", "CAB", "CAL", "CAM", "CAN", "CAP", "CAR", "CAT",
    "CAW", "COD", "COG", "COL", "CON", "COO", "COP", "COT", "COW", "COY",
    "CRY", "CUB", "CUE", "CUP", "CUR", "CUT", "DAB", "DAD", "DAM", "DAN",
    "DAR", "DAY", "DEE", "DEL", "DEN", "DES", "DEW", "DID", "DIE", "DIG",
    "DIN", "DIP", "DO", "DOE", "DOG", "DON", "DOT", "DOW", "DRY", "DUB",
    "DUD", "DUE", "DUG", "DUN", "EAR", "EAT", "ED", "EEL", "EGG", "EGO",
    "ELI", "ELK", "ELM", "ELY", "EM", "END", "EST", "ETC", "EVA", "EVE",
    "EWE", "EYE", "FAD", "FAN", "FAR", "FAT", "FAY", "FED", "FEE", "FEW",
    "FIB", "FIG", "FIN", "FIR", "FIT", "FLO", "FLY", "FOE", "FOG", "FOR",
    "FRY", "FUM", "FUN", "FUR", "GAB", "GAD", "GAG", "GAL", "GAM", "GAP",
    "GAS", "GAY", "GEE", "GEL", "GEM", "GET", "GIG", "GIL", "GIN", "GO",
    "GOT", "GUM", "GUN", "GUS", "GUT", "GUY", "GYM", "GYP", "HA", "HAD",
    "HAL", "HAM", "HAN", "HAP", "HAS", "HAT", "HAW", "HAY", "HE", "HEM",
    "HEN", "HER", "HEW", "HEY", "HI", "HID", "HIM", "HIP", "HIS", "HIT",
    "HO", "HOB", "HOC", "HOE", "HOG", "HOP", "HOT", "HOW", "HUB", "HUE",
    "HUG", "HUH", "HUM", "HUT", "I", "ICY", "IDA", "IF", "IKE", "ILL",
    "INK", "INN", "IO", "ION", "IQ", "IRA", "IRE", "IRK", "IS", "IT", "ITS",
    "IVY", "JAB", "JAG", "JAM", "JAN", "JAR", "JAW", "JAY", "JET", "JIG",
    "JIM", "JO", "JOB", "JOE", "JOG", "JOT", "JOY", "JUG", "JUT", "KAY",
    "KEG", "KEN", "KEY", "KID", "KIM", "KIN", "KIT", "LA", "LAB", "LAC",
    "LAD", "LAG", "LAM", "LAP", "LAW", "LAY", "LEA", "LED", "LEE", "LEG",
    "LEN", "LEO", "LET", "LEW", "LID", "LIE", "LIN", "LIP", "LIT", "LO",
    "LOB", "LOG", "LOP", "LOS", "LOT", "LOU", "LOW", "LOY", "LUG", "LYE",
    "MA", "MAC", "MAD", "MAE", "MAN", "MAO", "MAP", "MAT", "MAW", "MAY",
    "ME", "MEG", "MEL", "MEN", "MET", "MEW", "MID", "MIN", "MIT", "MOB",
    "MOD", "MOE", "MOO", "MOP", "MOS", "MOT", "MOW", "MUD", "MUG", "MUM",
    "MY", "NAB", "NAG", "NAN", "NAP", "NAT", "NAY", "NE", "NED", "NEE",
    "NET", "NEW", "NIB", "NIL", "NIP", "NIT", "NO", "NOB", "NOD", "NON",
    "NOR", "NOT", "NOV", "NOW", "NU", "NUN", "NUT", "O", "OAF", "OAK",
    "OAR", "OAT", "ODD", "ODE", "OF", "OFF", "OFT", "OH", "OIL", "OK",
    "OLD", "ON", "ONE", "OR", "ORB", "ORE", "ORR", "OS", "OTT", "OUR",
    "OUT", "OVA", "OW", "OWE", "OWL", "OWN", "OX", "PA", "PAD", "PAL",
    "PAM", "PAN", "PAP", "PAR", "PAT", "PAW", "PAY", "PEA", "PEG", "PEN",
    "PEP", "PER", "PET", "PEW", "PHI", "PI", "PIE", "PIN", "PIT", "PLY",
    "PO", "POD", "POE", "POP", "POT", "POW", "PRO", "PRY", "PUB", "PUG",
    "PUN", "PUP", "PUT", "QUO", "RAG", "RAM", "RAN", "RAP", "RAT", "RAW",
    "RAY", "REB", "RED", "REP", "RET", "RIB", "RID", "RIG", "RIM", "RIO",
    "RIP", "ROB", "ROD", "ROE", "RON", "ROT", "ROW", "ROY", "RUB", "RUE",
    "RUG", "RUM", "RUN", "RYE", "SAC", "SAD", "SAG", "SAL", "SAM", "SAN",
    "SAP", "SAT", "SAW", "SAY", "SEA", "SEC", "SEE", "SEN", "SET", "SEW",
    "SHE", "SHY", "SIN", "SIP", "SIR", "SIS", "SIT", "SKI", "SKY", "SLY",
    "SO", "SOB", "SOD", "SON", "SOP", "SOW", "SOY", "SPA", "SPY", "SUB",
    "SUD", "SUE", "SUM", "SUN", "SUP", "TAB", "TAD", "TAG", "TAN", "TAP",
    "TAR", "TEA", "TED", "TEE", "TEN", "THE", "THY", "TIC", "TIE", "TIM",
    "TIN", "TIP", "TO", "TOE", "TOG", "TOM", "TON", "TOO", "TOP", "TOW",
    "TOY", "TRY", "TUB", "TUG", "TUM", "TUN", "TWO", "UN", "UP", "US",
    "USE", "VAN", "VAT", "VET", "VIE", "WAD", "WAG", "WAR", "WAS", "WAY",
    "WE", "WEB", "WED", "WEE", "WET", "WHO", "WHY", "WIN", "WIT", "WOK",
    "WON", "WOO", "WOW", "WRY", "WU", "YAM", "YAP", "YAW", "YE", "YEA",
    "YES", "YET", "YOU", "ABED", "ABEL", "ABET", "ABLE", "ABUT", "ACHE",
    "ACID", "ACME", "ACRE", "ACTA", "ACTS", "ADAM", "ADDS", "ADEN", "AFAR",
    "AFRO", "AGEE", "AHEM", "AHOY", "AIDA", "AIDE", "AIDS", "AIRY", "AJAR",
    "AKIN", "ALAN", "ALEC", "ALGA", "ALIA", "ALLY", "ALMA", "ALOE", "ALSO",
    "ALTO", "ALUM", "ALVA", "AMEN", "AMES", "AMID", "AMMO", "AMOK", "AMOS",
    "AMRA", "ANDY", "ANEW", "ANNA", "ANNE", "ANTE", "ANTI", "AQUA", "ARAB",
    "ARCH", "AREA", "ARGO", "ARID", "ARMY", "ARTS", "ARTY", "ASIA", "ASKS",
    "ATOM", "AUNT", "AURA", "AUTO", "AVER", "AVID", "AVIS", "AVON", "AVOW",
    "AWAY", "AWRY", "BABE", "BABY", "BACH", "BACK", "BADE", "BAIL", "BAIT",
    "BAKE", "BALD", "BALE", "BALI", "BALK", "BALL", "BALM", "BAND", "BANE",
    "BANG", "BANK", "BARB", "BARD", "BARE", "BARK", "BARN", "BARR", "BASE",
    "BASH", "BASK", "BASS", "BATE", "BATH", "BAWD", "BAWL", "BEAD", "BEAK",
    "BEAM", "BEAN", "BEAR", "BEAT", "BEAU", "BECK", "BEEF", "BEEN", "BEER",
    "BEET", "BELA", "BELL", "BELT", "BEND", "BENT", "BERG", "BERN", "BERT",
    "BESS", "BEST", "BETA", "BETH", "BHOY", "BIAS", "BIDE", "BIEN", "BILE",
    "BILK", "BILL", "BIND", "BING", "BIRD", "BITE", "BITS", "BLAB", "BLAT",
    "BLED", "BLEW", "BLOB", "BLOC", "BLOT", "BLOW", "BLUE", "BLUM", "BLUR",
    "BOAR", "BOAT", "BOCA", "BOCK", "BODE", "BODY", "BOGY", "BOHR", "BOIL",
    "BOLD", "BOLO", "BOLT", "BOMB", "BONA", "BOND", "BONE", "BONG", "BONN",
    "BONY", "BOOK", "BOOM", "BOON", "BOOT", "BORE", "BORG", "BORN", "BOSE",
    "BOSS", "BOTH", "BOUT", "BOWL", "BOYD", "BRAD", "BRAE", "BRAG", "BRAN",
    "BRAY", "BRED", "BREW", "BRIG", "BRIM", "BROW", "BUCK", "BUDD", "BUFF",
    "BULB", "BULK", "BULL", "BUNK", "BUNT", "BUOY", "BURG", "BURL", "BURN",
    "BURR", "BURT", "BURY", "BUSH", "BUSS", "BUST", "BUSY", "BYTE", "CADY",
    "CAFE", "CAGE", "CAIN", "CAKE", "CALF", "CALL", "CALM", "CAME", "CANE",
    "CANT", "CARD", "CARE", "CARL", "CARR", "CART", "CASE", "CASH", "CASK",
    "CAST", "CAVE", "CEIL", "CELL", "CENT", "CERN", "CHAD", "CHAR", "CHAT",
    "CHAW", "CHEF", "CHEN", "CHEW", "CHIC", "CHIN", "CHOU", "CHOW", "CHUB",
    "CHUG", "CHUM", "CITE", "CITY", "CLAD", "CLAM", "CLAN", "CLAW", "CLAY",
    "CLOD", "CLOG", "CLOT", "CLUB", "CLUE", "COAL", "COAT", "COCA", "COCK",
    "COCO", "CODA", "CODE", "CODY", "COED", "COIL", "COIN", "COKE", "COLA",
    "COLD", "COLT", "COMA", "COMB", "COME", "COOK", "COOL", "COON", "COOT",
    "CORD", "CORE", "CORK", "CORN", "COST", "COVE", "COWL", "CRAB", "CRAG",
    "CRAM", "CRAY", "CREW", "CRIB", "CROW", "CRUD", "CUBA", "CUBE", "CUFF",
    "CULL", "CULT", "CUNY", "CURB", "CURD", "CURE", "CURL", "CURT", "CUTS",
    "DADE", "DALE", "DAME", "DANA", "DANE", "DANG", "DANK", "DARE", "DARK",
    "DARN", "DART", "DASH", "DATA", "DATE", "DAVE", "DAVY", "DAWN", "DAYS",
    "DEAD", "DEAF", "DEAL", "DEAN", "DEAR", "DEBT", "DECK", "DEED", "DEEM",
    "DEER", "DEFT", "DEFY", "DELL", "DENT", "DENY", "DESK", "DIAL", "DICE",
    "DIED", "DIET", "DIME", "DINE", "DING", "DINT", "DIRE", "DIRT", "DISC",
    "DISH", "DISK", "DIVE", "DOCK", "DOES", "DOLE", "DOLL", "DOLT", "DOME",
    "DONE", "DOOM", "DOOR", "DORA", "DOSE", "DOTE", "DOUG", "DOUR", "DOVE",
    "DOWN", "DRAB", "DRAG", "DRAM", "DRAW", "DREW", "DRUB", "DRUG", "DRUM",
    "DUAL", "DUCK", "DUCT", "DUEL", "DUET", "DUKE", "DULL", "DUMB", "DUNE",
    "DUNK", "DUSK", "DUST", "DUTY", "EACH", "EARL", "EARN", "EASE", "EAST",
    "EASY", "EBEN", "ECHO", "EDDY", "EDEN", "EDGE", "EDGY", "EDIT", "EDNA",
    "EGAN", "ELAN", "ELBA", "ELLA", "ELSE", "EMIL", "EMIT", "EMMA", "ENDS",
    "ERIC", "EROS", "EVEN", "EVER", "EVIL", "EYED", "FACE", "FACT", "FADE",
    "FAIL", "FAIN", "FAIR", "FAKE", "FALL", "FAME", "FANG", "FARM", "FAST",
    "FATE", "FAWN", "FEAR", "FEAT", "FEED", "FEEL", "FEET", "FELL", "FELT",
    "FEND", "FERN", "FEST", "FEUD", "FIEF", "FIGS", "FILE", "FILL", "FILM",
    "FIND", "FINE", "FINK", "FIRE", "FIRM", "FISH", "FISK", "FIST", "FITS",
    "FIVE", "FLAG", "FLAK", "FLAM", "FLAT", "FLAW", "FLEA", "FLED", "FLEW",
    "FLIT", "FLOC", "FLOG", "FLOW", "FLUB", "FLUE", "FOAL", "FOAM", "FOGY",
    "FOIL", "FOLD", "FOLK", "FOND", "FONT", "FOOD", "FOOL", "FOOT", "FORD",
    "FORE", "FORK", "FORM", "FORT", "FOSS", "FOUL", "FOUR", "FOWL", "FRAU",
    "FRAY", "FRED", "FREE", "FRET", "FREY", "FROG", "FROM", "FUEL", "FULL",
    "FUME", "FUND", "FUNK", "FURY", "FUSE", "FUSS", "GAFF", "GAGE", "GAIL",
    "GAIN", "GAIT", "GALA", "GALE", "GALL", "GALT", "GAME", "GANG", "GARB",
    "GARY", "GASH", "GATE", "GAUL", "GAUR", "GAVE", "GAWK", "GEAR", "GELD",
    "GENE", "GENT", "GERM", "GETS", "GIBE", "GIFT", "GILD", "GILL", "GILT",
    "GINA", "GIRD", "GIRL", "GIST", "GIVE", "GLAD", "GLEE", "GLEN", "GLIB",
    "GLOB", "GLOM", "GLOW", "GLUE", "GLUM", "GLUT", "GOAD", "GOAL", "GOAT",
    "GOER", "GOES", "GOLD", "GOLF", "GONE", "GONG", "GOOD", "GOOF", "GORE",
    "GORY", "GOSH", "GOUT", "GOWN", "GRAB", "GRAD", "GRAY", "GREG", "GREW",
    "GREY", "GRID", "GRIM", "GRIN", "GRIT", "GROW", "GRUB", "GULF", "GULL",
    "GUNK", "GURU", "GUSH", "GUST", "GWEN", "GWYN", "HAAG", "HAAS", "HACK",
    "HAIL", "HAIR", "HALE", "HALF", "HALL", "HALO", "HALT", "HAND", "HANG",
    "HANK", "HANS", "HARD", "HARK", "HARM", "HART", "HASH", "HAST", "HATE",
    "HATH", "HAUL", "HAVE", "HAWK", "HAYS", "HEAD", "HEAL", "HEAR", "HEAT",
    "HEBE", "HECK", "HEED", "HEEL", "HEFT", "HELD", "HELL", "HELM", "HERB",
    "HERD", "HERE", "HERO", "HERS", "HESS", "HEWN", "HICK", "HIDE", "HIGH",
    "HIKE", "HILL", "HILT", "HIND", "HINT", "HIRE", "HISS", "HIVE", "HOBO",
    "HOCK", "HOFF", "HOLD", "HOLE", "HOLM", "HOLT", "HOME", "HONE", "HONK",
    "HOOD", "HOOF", "HOOK", "HOOT", "HORN", "HOSE", "HOST", "HOUR", "HOVE",
    "HOWE", "HOWL", "HOYT", "HUCK", "HUED", "HUFF", "HUGE", "HUGH", "HUGO",
    "HULK", "HULL", "HUNK", "HUNT", "HURD", "HURL", "HURT", "HUSH", "HYDE",
    "HYMN", "IBIS", "ICON", "IDEA", "IDLE", "IFFY", "INCA", "INCH", "INTO",
    "IONS", "IOTA", "IOWA", "IRIS", "IRMA", "IRON", "ISLE", "ITCH", "ITEM",
    "IVAN", "JACK", "JADE", "JAIL", "JAKE", "JANE", "JAVA", "JEAN", "JEFF",
    "JERK", "JESS", "JEST", "JIBE", "JILL", "JILT", "JIVE", "JOAN", "JOBS",
    "JOCK", "JOEL", "JOEY", "JOHN", "JOIN", "JOKE", "JOLT", "JOVE", "JUDD",
    "JUDE", "JUDO", "JUDY", "JUJU", "JUKE", "JULY", "JUNE", "JUNK", "JUNO",
    "JURY", "JUST", "JUTE", "KAHN", "KALE", "KANE", "KANT", "KARL", "KATE",
    "KEEL", "KEEN", "KENO", "KENT", "KERN", "KERR", "KEYS", "KICK", "KILL",
    "KIND", "KING", "KIRK", "KISS", "KITE", "KLAN", "KNEE", "KNEW", "KNIT",
    "KNOB", "KNOT", "KNOW", "KOCH", "KONG", "KUDO", "KURD", "KURT", "KYLE",
    "LACE", "LACK", "LACY", "LADY", "LAID", "LAIN", "LAIR", "LAKE", "LAMB",
    "LAME", "LAND", "LANE", "LANG", "LARD", "LARK", "LASS", "LAST", "LATE",
    "LAUD", "LAVA", "LAWN", "LAWS", "LAYS", "LEAD", "LEAF", "LEAK", "LEAN",
    "LEAR", "LEEK", "LEER", "LEFT", "LEND", "LENS", "LENT", "LEON", "LESK",
    "LESS", "LEST", "LETS", "LIAR", "LICE", "LICK", "LIED", "LIEN", "LIES",
    "LIEU", "LIFE", "LIFT", "LIKE", "LILA", "LILT", "LILY", "LIMA", "LIMB",
    "LIME", "LIND", "LINE", "LINK", "LINT", "LION", "LISA", "LIST", "LIVE",
    "LOAD", "LOAF", "LOAM", "LOAN", "LOCK", "LOFT", "LOGE", "LOIS", "LOLA",
    "LONE", "LONG", "LOOK", "LOON", "LOOT", "LORD", "LORE", "LOSE", "LOSS",
    "LOST", "LOUD", "LOVE", "LOWE", "LUCK", "LUCY", "LUGE", "LUKE", "LULU",
    "LUND", "LUNG", "LURA", "LURE", "LURK", "LUSH", "LUST", "LYLE", "LYNN",
    "LYON", "LYRA", "MACE", "MADE", "MAGI", "MAID", "MAIL", "MAIN", "MAKE",
    "MALE", "MALI", "MALL", "MALT", "MANA", "MANN", "MANY", "MARC", "MARE",
    "MARK", "MARS", "MART", "MARY", "MASH", "MASK", "MASS", "MAST", "MATE",
    "MATH", "MAUL", "MAYO", "MEAD", "MEAL", "MEAN", "MEAT", "MEEK", "MEET",
    "MELD", "MELT", "MEMO", "MEND", "MENU", "MERT", "MESH", "MESS", "MICE",
    "MIKE", "MILD", "MILE", "MILK", "MILL", "MILT", "MIMI", "MIND", "MINE",
    "MINI", "MINK", "MINT", "MIRE", "MISS", "MIST", "MITE", "MITT", "MOAN",
    "MOAT", "MOCK", "MODE", "MOLD", "MOLE", "MOLL", "MOLT", "MONA", "MONK",
    "MONT", "MOOD", "MOON", "MOOR", "MOOT", "MORE", "MORN", "MORT", "MOSS",
    "MOST", "MOTH", "MOVE", "MUCH", "MUCK", "MUDD", "MUFF", "MULE", "MULL",
    "MURK", "MUSH", "MUST", "MUTE", "MUTT", "MYRA", "MYTH", "NAGY", "NAIL",
    "NAIR", "NAME", "NARY", "NASH", "NAVE", "NAVY", "NEAL", "NEAR", "NEAT",
    "NECK", "NEED", "NEIL", "NELL", "NEON", "NERO", "NESS", "NEST", "NEWS",
    "NEWT", "NIBS", "NICE", "NICK", "NILE", "NINA", "NINE", "NOAH", "NODE",
    "NOEL", "NOLL", "NONE", "NOOK", "NOON", "NORM", "NOSE", "NOTE", "NOUN",
    "NOVA", "NUDE", "NULL", "NUMB", "OATH", "OBEY", "OBOE", "ODIN", "OHIO",
    "OILY", "OINT", "OKAY", "OLAF", "OLDY", "OLGA", "OLIN", "OMAN", "OMEN",
    "OMIT", "ONCE", "ONES", "ONLY", "ONTO", "ONUS", "ORAL", "ORGY", "OSLO",
    "OTIS", "OTTO", "OUCH", "OUST", "OUTS", "OVAL", "OVEN", "OVER", "OWLY",
    "OWNS", "QUAD", "QUIT", "QUOD", "RACE", "RACK", "RACY", "RAFT", "RAGE",
    "RAID", "RAIL", "RAIN", "RAKE", "RANK", "RANT", "RARE", "RASH", "RATE",
    "RAVE", "RAYS", "READ", "REAL", "REAM", "REAR", "RECK", "REED", "REEF",
    "REEK", "REEL", "REID", "REIN", "RENA", "REND", "RENT", "REST", "RICE",
    "RICH", "RICK", "RIDE", "RIFT", "RILL", "RIME", "RING", "RINK", "RISE",
    "RISK", "RITE", "ROAD", "ROAM", "ROAR", "ROBE", "ROCK", "RODE", "ROIL",
    "ROLL", "ROME", "ROOD", "ROOF", "ROOK", "ROOM", "ROOT", "ROSA", "ROSE",
    "ROSS", "ROSY", "ROTH", "ROUT", "ROVE", "ROWE", "ROWS", "RUBE", "RUBY",
    "RUDE", "RUDY", "RUIN", "RULE", "RUNG", "RUNS", "RUNT", "RUSE", "RUSH",
    "RUSK", "RUSS", "RUST", "RUTH", "SACK", "SAFE", "SAGE", "SAID", "SAIL",
    "SALE", "SALK", "SALT", "SAME", "SAND", "SANE", "SANG", "SANK", "SARA",
    "SAUL", "SAVE", "SAYS", "SCAN", "SCAR", "SCAT", "SCOT", "SEAL", "SEAM",
    "SEAR", "SEAT", "SEED", "SEEK", "SEEM", "SEEN", "SEES", "SELF", "SELL",
    "SEND", "SENT", "SETS", "SEWN", "SHAG", "SHAM", "SHAW", "SHAY", "SHED",
    "SHIM", "SHIN", "SHOD", "SHOE", "SHOT", "SHOW", "SHUN", "SHUT", "SICK",
    "SIDE", "SIFT", "SIGH", "SIGN", "SILK", "SILL", "SILO", "SILT", "SINE",
    "SING", "SINK", "SIRE", "SITE", "SITS", "SITU", "SKAT", "SKEW", "SKID",
    "SKIM", "SKIN", "SKIT", "SLAB", "SLAM", "SLAT", "SLAY", "SLED", "SLEW",
    "SLID", "SLIM", "SLIT", "SLOB", "SLOG", "SLOT", "SLOW", "SLUG", "SLUM",
    "SLUR", "SMOG", "SMUG", "SNAG", "SNOB", "SNOW", "SNUB", "SNUG", "SOAK",
    "SOAR", "SOCK", "SODA", "SOFA", "SOFT", "SOIL", "SOLD", "SOME", "SONG",
    "SOON", "SOOT", "SORE", "SORT", "SOUL", "SOUR", "SOWN", "STAB", "STAG",
    "STAN", "STAR", "STAY", "STEM", "STEW", "STIR", "STOW", "STUB", "STUN",
    "SUCH", "SUDS", "SUIT", "SULK", "SUMS", "SUNG", "SUNK", "SURE", "SURF",
    "SWAB", "SWAG", "SWAM", "SWAN", "SWAT", "SWAY", "SWIM", "SWUM", "TACK",
    "TACT", "TAIL", "TAKE", "TALE", "TALK", "TALL", "TANK", "TASK", "TATE",
    "TAUT", "TEAL", "TEAM", "TEAR", "TECH", "TEEM", "TEEN", "TEET", "TELL",
    "TEND", "TENT", "TERM", "TERN", "TESS", "TEST", "THAN", "THAT", "THEE",
    "THEM", "THEN", "THEY", "THIN", "THIS", "THUD", "THUG", "TICK", "TIDE",
    "TIDY", "TIED", "TIER", "TILE", "TILL", "TILT", "TIME", "TINA", "TINE",
    "TINT", "TINY", "TIRE", "TOAD", "TOGO", "TOIL", "TOLD", "TOLL", "TONE",
    "TONG", "TONY", "TOOK", "TOOL", "TOOT", "TORE", "TORN", "TOTE", "TOUR",
    "TOUT", "TOWN", "TRAG", "TRAM", "TRAY", "TREE", "TREK", "TRIG", "TRIM",
    "TRIO", "TROD", "TROT", "TROY", "TRUE", "TUBA", "TUBE", "TUCK", "TUFT",
    "TUNA", "TUNE", "TUNG", "TURF", "TURN", "TUSK", "TWIG", "TWIN", "TWIT",
    "ULAN", "UNIT", "URGE", "USED", "USER", "USES", "UTAH", "VAIL", "VAIN",
    "VALE", "VARY", "VASE", "VAST", "VEAL", "VEDA", "VEIL", "VEIN", "VEND",
    "VENT", "VERB", "VERY", "VETO", "VICE", "VIEW", "VINE", "VISE", "VOID",
    "VOLT", "VOTE", "WACK", "WADE", "WAGE", "WAIL", "WAIT", "WAKE", "WALE",
    "WALK", "WALL", "WALT", "WAND", "WANE", "WANG", "WANT", "WARD", "WARM",
    "WARN", "WART", "WASH", "WAST", "WATS", "WATT", "WAVE", "WAVY", "WAYS",
    "WEAK", "WEAL", "WEAN", "WEAR", "WEED", "WEEK", "WEIR", "WELD", "WELL",
    "WELT", "WENT", "WERE", "WERT", "WEST", "WHAM", "WHAT", "WHEE", "WHEN",
    "WHET", "WHOA", "WHOM", "WICK", "WIFE", "WILD", "WILL", "WIND", "WINE",
    "WING", "WINK", "WINO", "WIRE", "WISE", "WISH", "WITH", "WOLF", "WONT",
    "WOOD", "WOOL", "WORD", "WORE", "WORK", "WORM", "WORN", "WOVE", "WRIT",
    "WYNN", "YALE", "YANG", "YANK", "YARD", "YARN", "YAWL", "YAWN", "YEAH",
    "YEAR", "YELL", "YOGA", "YOKE"
};

/*从char数组's'中提取'length'位
   从“开始”位开始*/

unsigned long RFC1751::extract (char const* s, int start, int length)
{
    unsigned char cl;
    unsigned char cc;
    unsigned char cr;
    unsigned long x;

    assert (length <= 11);
    assert (start >= 0);
    assert (length >= 0);
    assert (start + length <= 66);

    int const shiftR = 24 - (length + (start % 8));
cl = s[start / 8];                      //获取组件
    cc = (shiftR < 16) ? s[start / 8 + 1] : 0;
    cr = (shiftR < 8) ? s[start / 8 + 2] : 0;

x = ((long) (cl << 8 | cc) << 8 | cr) ; //把钻头放在一起
x = x >> shiftR; //右对齐数字
x = ( x & (0xffff >> (16 - length) ) ); //修剪多余的位。

    return x;
}

//将“c”中的8个字节编码为英语单词字符串。
//返回指向静态缓冲区的指针
void RFC1751::btoe (std::string& strHuman, std::string const& strData)
{
    /*r cabuffer[9]；/*为奇偶校验增加空间2位*/
    In p，I；

    memcpy（cabuffer，strdata.c_str（），8）；

    //计算奇偶校验：只需添加两位的组。
    对于（p=0，i=0；i<64；i+=2）
        P+=提取物（cabuffer，i，2）；

    cabuffer[8]=char（p）<<6；

    strhuman=std:：string（）。
                  +s_字典[摘录（cabuffer，0，11）]+“
                  +s_字典[摘录（cabuffer，11，11）]+“
                  +s_字典[摘录（cabuffer，22，11）]+“
                  +s_字典[摘录（cabuffer，33，11）]+“
                  +s_字典[摘录（cabuffer，44，11）]+“
                  +s_字典[摘录（cabuffer，55，11）]
}

void rfc1751:：insert（char*s，int x，int start，int length）
{
    无符号字符cl；
    无符号字符cc；
    无符号字符CR；
    无符号长Y；
    INT移位；

    断言（长度<=11）；
    断言（开始>=0）；
    断言（长度大于等于0）；
    断言（开始+长度<=66）；

    shift=（8-（（start+length）%8））%8）；
    y=（长）x<<shift；
    cl=（y>>16）&0xff；
    cc=（y>>8）&0xff；
    Cr＝y和0xFF；

    如果（移位+长度>16）
    {
        S[开始/8]=cl；
        S[开始/8+1]=cc；
        S[开始/8+2]=CR；
    }
    否则，如果（移位+长度>8）
    {
        S[开始/8]=cc；
        S[开始/8+1]=CR；
    }
    其他的
    {
        s[start/8]=cr；
    }
}

void rfc1751:：standard（std:：string&strword）
{
    for（自动&字母：strword）
    {
        if（islower（static_cast<unsigned char>（letter）））
            letter=toupper（static_cast<unsigned char>（letter））；
        else if（字母='1'）
            字母=“L”；
        else if（字母='0'）
            字母=“o”；
        else if（字母='5'）
            字母= s’；
    }
}

//字典的二进制搜索。
int rfc1751:：wsrch（std:：string const&strword，int imin，int imax）
{
    内部结果=-1；

    同时（结果<0&&imin！= IMAX）
    {
        //有一个搜索范围。
        int-imi=imin+（imax-imin）/2；
        int idir=strword.compare（s_字典[midi]）；

        如果（！）IDIR）
        {
            iresult=midi；//找到了。
        }
        否则，如果（idir<0）
        {
            imax=midi；//key<middle，middle是新的max。
        }
        其他的
        {
            imin=imi+1；//key>middle，new min超过middle。
        }
    }

    返回结果；
}

//将6个字转换为二进制。
/ /
//返回1 OK-所有好字和奇偶校验都正常
//数据库中没有0个字
//-1输入ie>4个字符的单词格式错误
//-2个字正常，但奇偶校验错误
int rfc1751:：etob（std:：string&strdata，std:：vector<std:：string>vshuman）
{
    INTI，P，V，L；
    炭B〔9〕；

    如果（6）！=vshuman.size（））
        返回- 1；

    memset（b，0，sizeof（b））；

    P＝0；
    用于（auto&strword:vshuman）
    {
        L=strword.length（）；

        如果（l>4 l<1）
            返回- 1；

        标准（strword）；

        V=Wsrch（字符串，
                   L＜4？0：571，
                   L＜4？570：2048）；

        如果（V＜0）
            返回0；

        插入（b，v，p，11）；
        p+＝11；
    }

    /*现在检查一下我们得到的东西的奇偶性*/

    for (p = 0, i = 0; i < 64; i += 2)
        p += extract (b, i, 2);

    if ( (p & 3) != extract (b, 64, 2) )
        return -2;

    strData.assign (b, 8);

    return 1;
}

/*将按空格分隔的单词转换为高位优先的128位键格式。

    @返回
         1如果成功
         如果单词不在词典中，则为0
        如果字符串格式不正确，则为-1
        -2如果单词正常，但奇偶校验错误。
**/

int RFC1751::getKeyFromEnglish (std::string& strKey, std::string const& strHuman)
{
    std::vector<std::string> vWords;
    std::string strFirst, strSecond;
    int rc = 0;

    std::string strTrimmed (strHuman);

    boost::algorithm::trim (strTrimmed);

    boost::algorithm::split (vWords, strTrimmed,
                             boost::algorithm::is_space (), boost::algorithm::token_compress_on);

    rc  = 12 == vWords.size () ? 1 : -1;

    if (1 == rc)
        rc = etob (strFirst, vWords | boost::adaptors::copied (0, 6));

    if (1 == rc)
        rc = etob (strSecond, vWords | boost::adaptors::copied (6, 12));

    if (1 == rc)
        strKey  = strFirst + strSecond;

    return rc;
}

/*从128位的big-endian格式的密钥转换为human格式
**/

void RFC1751::getEnglishFromKey (std::string& strHuman, std::string const& strKey)
{
    std::string strFirst, strSecond;

    btoe (strFirst, strKey.substr (0, 8));
    btoe (strSecond, strKey.substr (8, 8));

    strHuman    = strFirst + " " + strSecond;
}

std::string
RFC1751::getWordFromBlob (void const* blob, size_t bytes)
{
//这是Jenkins一次一个哈希的简单实现
//算法：
//http://en.wikipedia.org/wiki/jenkins-hash-u函数一次一个
    unsigned char const* data = static_cast<unsigned char const*>(blob);
    std::uint32_t hash = 0;

    for (size_t i = 0; i < bytes; ++i)
    {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return s_dictionary [hash % (sizeof (s_dictionary) / sizeof (s_dictionary [0]))];
}

} //涟漪
