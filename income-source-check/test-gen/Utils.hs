module Utils where


import Data.List
import Data.Word
import System.Random


replicateAction
  :: Integer -> IO a -> IO [a]
replicateAction n act
  = sequence $ genericReplicate n $ act

genRandomString
  :: Integer -> IO String
genRandomString n
  = replicateAction n $ randomRIO ('A' , 'Z')

genRandomNumeral
  :: Integer -> IO String
genRandomNumeral n
  = replicateAction n $ randomRIO ('0' , '9')

genRandomStringUpTo
  :: Integer -> IO String
genRandomStringUpTo n
  = do
      l <- randomRIO (1 , n)
      genRandomString l

genRandomNumeralUpTo
  :: Integer -> IO String
genRandomNumeralUpTo n
  = do
      l <- randomRIO (1 , n)
      genRandomNumeral l

genRandomBStr
  :: Integer -> IO [Word8]
genRandomBStr n
  = replicateAction n $ randomRIO (minBound , maxBound)

isSingleton
  :: [a] -> Bool
isSingleton [_]
  = True
isSingleton _
  = False

-- Assumes the argument being sorted
sortedIsContainingDuplicates
  :: (Ord a)
  => [a] -> Bool
sortedIsContainingDuplicates
  = not . all isSingleton . group

