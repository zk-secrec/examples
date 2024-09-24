module CBOR where


import Data.List
import Data.Map hiding (foldr, take)
import Numeric
import Data.Word
import Data.Time
import Data.Time.Format.ISO8601


base b n
  = let
      (q , r)
        = divMod n b
    in
    if n == 0
      then []
      else r : base b q


data CBORVal
  = CBORObjVal CBORObj
  | CBORArrVal CBORArr
  | CBORDatVal CBORDat
  | CBORTStrVal CBORTStr
  | CBORBStrVal CBORBStr
  | CBORNumVal CBORNum
  deriving (Eq, Ord, Show)

data CBORObj
  = CBORStrMapObj (Map CBORTStr CBORVal)
  | CBORNumMapObj (Map CBORNum CBORVal)
  deriving (Eq, Ord, Show)

newtype CBORArr
  = CBORLstArr [CBORVal]
  deriving (Eq, Ord, Show)

newtype CBORDat
  = CBORStrDat CBORTStr
  deriving (Eq, Ord, Show)

newtype CBORTStr
  = CBORTStr [Word8]
  deriving (Eq, Ord, Show)

newtype CBORBStr
  = CBORBStr [Word8]
  deriving (Eq, Ord, Show)

newtype CBORNum
  = CBORNum Integer
  deriving (Eq, Ord, Show)


addType
  :: Word8 -> [Word8] -> [Word8]
addType t (b : bs)
  | t < 8
    = b + t * 32 : bs
  | otherwise
    = error ("addType: type " ++ show t ++ " is invalid")


class CBOR a where
  
  cbor
    :: a -> [Word8]
  
  cbors
    :: a -> [Word8] -> [Word8]
  
  cbors val cont
    = cbor val ++ cont
  
  cbor val
    = cbors val []
  

instance CBOR CBORNum where
  
  cbor (CBORNum num)
    | num < 24
      = [fromInteger num]
    | len <= 1
      = ord 0
    | len <= 2
      = ord 1
    | len <= 4
      = ord 2
    | len <= 8
      = ord 3
    | otherwise
      = error ("Number " ++ show num ++ " not representable as CBOR")
    where
      bytes
        = fmap fromInteger (base 256 num)
      len
        = length bytes
      ord k
        = [24 + k] ++ replicate (2 ^ k - len) 0 ++ reverse bytes
  

instance CBOR CBORTStr where
  
  cbors (CBORTStr str) cont
    = addType 3 (cbors (CBORNum val) (str ++ cont))
    where
      val
        = toInteger (length str)
  

instance CBOR CBORBStr where
  
  cbors (CBORBStr str) cont
    = addType 2 (cbors (CBORNum val) (str ++ cont))
    where
      val
        = toInteger (length str)
  

instance CBOR CBORDat where
  
  cbors (CBORStrDat cborstr) cont
    = addType 6 (cbors (CBORNum val) (cbors cborstr cont))
    where
      val
        = 18013
  

instance CBOR CBORArr where
  
  cbors (CBORLstArr cborvals) cont
    = addType 4 (cbors (CBORNum val) (foldr cbors cont cborvals))
    where
      val
        = toInteger (length cborvals)
  

instance CBOR CBORObj where
  
  cbors (CBORStrMapObj cbormap) cont
    = addType 5 (cbors (CBORNum val) (foldr (\ (k , v) -> cbors k . cbors v) cont (assocs cbormap)))
    where
      val
        = toInteger (size cbormap)
  cbors (CBORNumMapObj cbormap) cont
    = addType 5 (cbors (CBORNum val) (foldr (\ (k , v) -> cbors k . cbors v) cont (assocs cbormap)))
    where
      val
        = toInteger (size cbormap)

instance CBOR CBORVal where
  
  cbors (CBORObjVal cborobj)
    = cbors cborobj
  cbors (CBORArrVal cborarr)
    = cbors cborarr
  cbors (CBORDatVal cbordat)
    = cbors cbordat
  cbors (CBORTStrVal cborstr)
    = cbors cborstr
  cbors (CBORBStrVal cborstr)
    = cbors cborstr
  cbors (CBORNumVal cbornum)
    = cbors cbornum
  

cborKey
  :: String -> CBORTStr
cborKey
  = CBORTStr . fmap (fromIntegral . fromEnum)

cborStr
  :: String -> CBORVal
cborStr
  = CBORTStrVal . cborKey

cborISO8601
  :: String -> CBORVal
cborISO8601
  = CBORDatVal . CBORStrDat . cborKey

cborDat
  :: Day -> CBORVal
cborDat
  = cborISO8601 . formatShow iso8601Format

showCBOR
  :: (CBOR a)
  => a -> String
showCBOR
  = foldr showHex "" . cbor
