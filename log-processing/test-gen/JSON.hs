{-# LANGUAGE TypeSynonymInstances #-}
{-# LANGUAGE FlexibleInstances #-}
module JSON where


import Data.Char
import Data.Map hiding (null)
import System.FilePath


data JSONVal
  = JSONObjVal JSONObj
  | JSONArrVal JSONArr
  | JSONNumVal JSONNum
  | JSONStrVal JSONStr
  | JSONTruVal
  | JSONFlsVal
  | JSONNulVal
  deriving (Eq, Ord)

newtype JSONObj
  = JSONMapObj (Map JSONStr JSONVal)
  deriving (Eq, Ord)

newtype JSONArr
  = JSONLstArr [JSONVal]
  deriving (Eq, Ord)

type JSONNum
  = Integer

type JSONStr
  = String


class Pretty a where
  
  pretty
    :: Int -> a -> String
  
  prettys
    :: Int -> a -> String -> String
  
  prettys ind val cont
    = pretty ind val ++ cont
  
  pretty ind val
    = prettys ind val ""
  

instance Pretty Integer where
  
  prettys ind num cont
    = shows num cont
  

instance {-# OVERLAPPING #-} Pretty String where
  
  prettys ind str cont
    = shows str cont
  

instance {-# OVERLAPPABLE #-} (Pretty a) => Pretty [a] where
  
  prettys ind xs cont
    = let
        nl
          = "\n" ++ replicate ind ' '
        prettyHelper (x : xs) cont 
          = prettys (ind + 2) x $ 
            nl ++
            if null xs then cont else ", " ++ prettyHelper xs cont
        prettyHelper _        cont
          = nl ++ cont
      in
      nl ++ "[ " ++ prettyHelper xs ("] " ++ cont)

instance (Pretty a) => Pretty (Map String a) where
  
  prettys ind map cont
    = let
        nl
          = "\n" ++ replicate ind ' '
        prettyHelper ((k , v) : ps) cont
          = prettys (ind + 2) k $
            (": " ++) $ 
            prettys (ind + 2) v $ 
            nl ++
            if null ps then cont else ", " ++ prettyHelper ps cont
        prettyHelper _              cont
          = nl ++ cont
      in
      nl ++ "{ " ++ prettyHelper (assocs map) ("} " ++ cont)
  

instance Pretty JSONArr where
  
  prettys ind (JSONLstArr lst)
    = prettys ind lst
  

instance Pretty JSONObj where
  
  prettys ind (JSONMapObj map)
    = prettys ind map
  

instance Pretty JSONVal where
  
  prettys ind (JSONObjVal obj)
    = prettys ind obj
  prettys ind (JSONArrVal arr)
    = prettys ind arr
  prettys ind (JSONNumVal num)
    = prettys ind num
  prettys ind (JSONStrVal str)
    = prettys ind str
  prettys ind JSONTruVal
    = ("true " ++)
  prettys ind JSONFlsVal
    = ("false " ++)
  prettys ind JSONNulVal
    = ("null " ++)
  

class JSONable a where
  
  adapt
    :: a -> JSONVal
  

instance JSONable Integer where
  
  adapt n
    = JSONStrVal (show n)
  

instance JSONable String where
  
  adapt str
    = JSONArrVal (JSONLstArr (fmap (adapt . toInteger . fromEnum) str))
  

instance JSONable JSONArr where
  
  adapt (JSONLstArr xs)
    = JSONArrVal (JSONLstArr (fmap adapt xs))
  

instance JSONable JSONObj where
  
  adapt (JSONMapObj map)
    = JSONObjVal (JSONMapObj (Data.Map.map adapt map))
  

instance JSONable JSONVal where
  
  adapt (JSONObjVal obj)
    = adapt obj
  adapt (JSONArrVal arr)
    = adapt arr
  adapt (JSONNumVal num)
    = adapt num
  adapt (JSONStrVal str)
    = adapt str
  adapt jval
    = jval
  

writeJSON
  :: FilePath -> String -> [Int] -> JSONVal -> IO ()
writeJSON dir kind param json
  = writeFile (dir </> kind ++ (param >>= show) <.> "json") $ dropWhile isSpace $ pretty 0 $ json


