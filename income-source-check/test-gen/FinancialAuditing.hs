{-# LANGUAGE StandaloneDeriving #-}
{-# LANGUAGE GeneralizedNewtypeDeriving #-}
module FinancialAuditing where


import Control.Applicative hiding (empty)
import Control.Monad
import Data.List
import Data.Map hiding (foldr)
import Data.Time
import Data.Time.Format.ISO8601
import System.Random
import System.Environment
import System.Directory

import JSON
import Utils


deriving instance Random Day


transferNumFactor = 2 ^ 8

decimalSeparator = '.'
placeHolder = 0
currency = "EUR"

clientPos = 0
datePos = 2
beneficiaryPos = 3
dcPos = 7
amountPos = 8

posLength = 15
amountLength = 15

goodCountries = ["EE"]
badCountries = ["PL", "VA"]

bigAmount = 200000
goodFloor = 50
badCeiling = 10
bigCeiling = 60

bgnPeriod = fromGregorian 2022 4 1
endPeriod = fromGregorian 2022 4 30

client = "EE241010102018120006"

title
  = intercalate "," $ 
    [ "Account"
    , "Document No."
    , "Date"
    , "Beneficiary's account"
    , "Beneficiary's name"
    , "BIC/SWIFT"
    , "Type"
    , "(D/C)"
    , "Amount"
    , "Reference No."
    , "Archive ID"
    , "Description"
    , "Commission fee"
    , "Currency"
    , "ID code/Registry code"
    ]

maxlenAccount = genericLength client + 2
maxlenDocumentNo = 14
maxlenDate = 12
maxlenName = 32
maxlenBIC = 10
maxlenType = 4
maxlenDC = 3
maxlenAmount = 15
maxlenReferenceNo = 16
maxlenArchiveID = 14
maxlenDescription = 64
maxlenCommissionFee = 1
maxlenCurrency = genericLength currency + 2

totallenCells = sum [maxlenAccount, maxlenDocumentNo, maxlenDate, maxlenAccount, maxlenName, maxlenBIC, maxlenType, maxlenDC, maxlenAmount, maxlenReferenceNo, maxlenArchiveID, maxlenDescription, maxlenCommissionFee, maxlenCurrency]

genDocumentNo
  = fmap show $ genRandomStringUpTo (maxlenDocumentNo - 2)

genDate
  = fmap show $ do
      date <- fmap (formatShow iso8601Format) (randomRIO (fromGregorian 2022 1 1 , fromGregorian 2022 11 30))
      case date of
        [y1, y2, y3, y4, _, m1, m2, _, d1, d2]
          -> return [d1, d2, '.', m1, m2, '.', y1, y2, y3, y4]

genAccount
  = fmap show $ do
      country <- return "EE"
      number <- genRandomNumeral (maxlenAccount - 4)
      return (country ++ number)

genName
  = fmap show $ genRandomStringUpTo (maxlenName - 2)

genBIC
  = fmap show $ genRandomString (maxlenBIC - 2)

genType
  = fmap show $ genRandomString (maxlenType - 2)

genDC
  = fmap show $ fmap (return :: Char -> String) (randomRIO ('C' , 'D'))

genAmount
  = do
      integralPart <- randomRIO (0 :: Int , 1999)
      fractionalPart <- randomRIO (0 :: Int , 99)
      return (show integralPart ++ decimalSeparator : show fractionalPart)

genReferenceNo
  = genRandomNumeralUpTo (maxlenReferenceNo - 2)

genArchiveID
  = fmap show $ do
      head <- genRandomString 2
      tail <- genRandomNumeral (maxlenArchiveID - 4)
      return (head ++ tail)

genDescription
  = fmap show $ genRandomStringUpTo (maxlenDescription - 2)

genCode currentLen
  = fmap show $ genRandomNumeral $ totallenCells - currentLen

genTransfer account
  = do
      things <- sequence [return (show account), genDocumentNo, genDate, genAccount, genName, genBIC, genType, genDC, genAmount, genReferenceNo, genArchiveID, genDescription, return "0", return (show "EUR")]
      code <- genCode (sum (fmap genericLength things))
      return $ intercalate "," $ things ++ [code]

main
  = do
      args <- getArgs
      guard (length args == 2) <|> fail "Expected exactly 2 arguments (output directory and size parameter)"
      let outputDir = args !! 0
      let paramLog = read (args !! 1)
      let param = 2 ^ paramLog
      let transferNum = param * transferNumFactor
      transfers <- sequence $ genericReplicate transferNum $ genTransfer client
      let stmt = foldr (\ row acc -> row ++ '\n' : acc) "" (title : transfers)
      let stmtRowLengths = genericLength title : genericReplicate transferNum (totallenCells + posLength + 1)
      let stmtLength = transferNum + 1
      let stmtCharsLength = sum stmtRowLengths + stmtLength
      let publ = adapt (JSONObjVal (JSONMapObj (fromList (zip ["client_pos", "date_pos", "beneficiary_pos", "dc_pos", "amount_pos"] (fmap JSONNumVal [clientPos, datePos, beneficiaryPos, dcPos, amountPos]) ++ zip ["decimal_separator", "placeholder"] (fmap JSONNumVal [toInteger (fromEnum decimalSeparator), placeHolder]) ++ [("client" , JSONStrVal client)] ++ zip ["amount_length", "pos_length", "stmt_length", "stmtchars_length"] (fmap JSONNumVal [amountLength, posLength, stmtLength, stmtCharsLength]) ++ zip ["good_countries", "bad_countries"] (fmap (JSONArrVal . JSONLstArr . fmap JSONStrVal) [goodCountries, badCountries]) ++ zip ["big_amount", "good_floor", "bad_ceiling", "big_ceiling"] (fmap JSONNumVal [bigAmount, goodFloor, badCeiling, bigCeiling]) ++ (zip ["bgn_year", "bgn_month", "bgn_day", "end_year", "end_month", "end_day"] $ let ((yyyy1 , mm1, dd1) , (yyyy2 , mm2 , dd2)) = (toGregorian bgnPeriod , toGregorian endPeriod) in fmap JSONNumVal [yyyy1, toInteger mm1, toInteger dd1, yyyy2, toInteger mm2, toInteger dd2]) ++ [("stmtrow_lengths" , JSONArrVal (JSONLstArr (fmap JSONNumVal stmtRowLengths)))]))))
      let inst = adapt (JSONObjVal (JSONMapObj empty))
      let witn = adapt (JSONObjVal (JSONMapObj (fromList [("stmtchars" , JSONStrVal stmt)])))
      createDirectoryIfMissing True outputDir
      writeJSON outputDir "public" [paramLog] publ
      writeJSON outputDir "instance" [paramLog] inst
      writeJSON outputDir "witness" [paramLog] witn

