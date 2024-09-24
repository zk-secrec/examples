module Cybersecurity where


import Control.Applicative
import Control.Monad
import Data.List
import Data.Map hiding (splitAt)
import System.Random
import System.Directory
import System.Environment

import JSON
import Utils


textLenFactor = 2 ^ 16
iocLen = 2 ^ 9
canaryLen = 2 ^ 9
iocNum = 1
canaryNum = 1


genIOC
  :: Integer -> IO [Char]
genIOC len
  = do
      letters <- genRandomString (len - 1)
      nonletter <- fmap toEnum $ randomRIO (33 , 64)
      pos <- randomRIO (0 , len - 1)
      let (us , vs) = splitAt (fromInteger pos) letters
      return (us ++ nonletter : vs)

genCanary
  :: Integer -> [a] -> IO (Integer , [a])
genCanary len text
  = do
      pos <- randomRIO (0 , genericLength text - len)
      return (pos , genericTake len (genericDrop pos text))

replicateJSON
  :: Integer -> JSONVal -> JSONVal
replicateJSON n json
  = JSONArrVal (JSONLstArr (genericReplicate n json))

main
  = do
      args <- getArgs
      guard (length args == 2) <|> fail "Expected exactly 2 arguments (output directory and size parameter)"
      let outputDir = args !! 0
      let paramLog = read (args !! 1)
      let param = 2 ^ paramLog
      let textLen = toInteger (param * textLenFactor)
      text <- genRandomString textLen
      iocList <- replicateAction iocNum $ genIOC iocLen
      canaryList <- replicateAction canaryNum $ genCanary canaryLen text
      let publ = adapt (JSONMapObj (fromList [("text_len" , JSONNumVal textLen), ("ioc_len_list" , replicateJSON iocNum (JSONNumVal iocLen)), ("canary_len_list" , replicateJSON canaryNum (JSONNumVal canaryLen))]))
      let inst = adapt (JSONMapObj (fromList [("ioc_list" , JSONArrVal (JSONLstArr (fmap JSONStrVal iocList))), ("canary_list" , JSONArrVal (JSONLstArr (fmap JSONStrVal (fmap snd canaryList))))]))
      let witn = adapt (JSONMapObj (fromList [("text" , JSONStrVal text), ("text_pos_list" , JSONArrVal (JSONLstArr (fmap JSONNumVal (fmap fst canaryList))))]))
      createDirectoryIfMissing True outputDir
      writeJSON outputDir "public" [paramLog] publ
      writeJSON outputDir "instance" [paramLog] inst
      writeJSON outputDir "witness" [paramLog] witn

