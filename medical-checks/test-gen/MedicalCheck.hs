{-# LANGUAGE TypeSynonymInstances #-}
{-# LANGUAGE FlexibleInstances #-}
module MedicalCheck where

import Control.Applicative
import Control.Monad
import Data.Char
import Data.List
import Data.Map hiding (foldr)
import System.Random
import System.Environment
import System.Directory
import System.FilePath
import Data.Word
import Data.Time
import Data.Time.Format.ISO8601
import qualified Data.ByteString
import Crypto.Hash.SHA256 hiding (init)

import JSON
import CBOR
import Utils


maxlenNpi = 7
maxlenDiagnosis = 7
numRevealedDiagnoses = 0

givenName = "Matt Michael"
familyName = "Murdock"
dob = formatShow iso8601Format $ fromGregorian 1985 1 14
revealedDiagnoses = []

issued = fromGregorian 2022 11 30

healthReportKeys = ["family_name", "given_name", "birth_date", "issue_date", "expiry_date", "issuing_authority", "diagnoses"]
msoKeys = ["digestAlgorithm", "valueDigests", "docType", "validityInfo"]
validityInfoKeys = ["signed", "validFrom", "validUntil"]
issuerSignedItemKeys = ["digestID", "random", "elementIdentifier", "elementValue"]

digestIDs = [1, 2, 3, 4, 5, 7, 34]

genRandomTests
  :: Integer -> Integer -> IO [String]
genRandomTests numtests n
  = replicateAction numtests $ genRandomStringUpTo n

genIssuerSignedItem
  :: Integer -> (CBORTStr , CBORVal) -> IO CBORVal
genIssuerSignedItem n (key , val)
  = do
      let digestIDVal = CBORNumVal (CBORNum n)
      randomVal <- fmap (CBORBStrVal . CBORBStr) (genRandomBStr 16)
      let elementKeyVal = CBORTStrVal key
      let elementValVal = val
      return $ CBORObjVal $ CBORStrMapObj $ fromList $ zip (fmap cborKey issuerSignedItemKeys) $
        [ digestIDVal
        , randomVal
        , elementKeyVal
        , elementValVal 
        ]
         
digest
  :: [Word8] -> [Word8]
digest
  = Data.ByteString.unpack . hash . Data.ByteString.pack

genMSO
  :: [[Word8]] -> (CBORVal , CBORVal , CBORVal) -> CBORVal
genMSO isis (signedVal , validFromVal , validUntilVal)
  = let
      digestAlgorithmVal = cborStr "SHA-256"
      valueDigestsVal = let
                          key = cborKey "org.iso.18013.5.1"
                          val = CBORObjVal $ CBORNumMapObj $ fromList $ 
                                zip (fmap CBORNum digestIDs) (fmap (CBORBStrVal . CBORBStr . digest) isis)
                        in
                        CBORObjVal $ CBORStrMapObj $ fromList $
                        [ (cborKey "nameSpaces" , CBORObjVal $ CBORStrMapObj $ fromList $ [ (key , val) ])
                        ]
      docTypeVal = cborStr "org.iso.18013.5.1.mDL"
      validityInfoVal = CBORObjVal $ CBORStrMapObj $ fromList $ zip (fmap cborKey validityInfoKeys) $
                        [ signedVal
                        , validFromVal
                        , validUntilVal
                        ]
    in
    CBORObjVal $ CBORStrMapObj $ fromList $ zip (fmap cborKey msoKeys) $
    [ digestAlgorithmVal
    , valueDigestsVal
    , docTypeVal
    , validityInfoVal
    ]

genHealthReportItems
  :: [String] -> [String] -> (CBORVal , CBORVal) -> IO [CBORVal]
genHealthReportItems goodProviders diagnoses (issueDateVal , expiryDateVal)
  = do
      let familyNameVal = cborStr familyName
      let givenNameVal = cborStr givenName
      let birthDateVal = CBORDatVal (CBORStrDat (cborKey dob))
      issuingAuthorityVal <- fmap (CBORTStrVal . CBORTStr . fmap (fromIntegral . fromEnum) . (goodProviders !!)) $ randomRIO (0 , length goodProviders - 1)
      let diagnosesVal = (CBORArrVal . CBORLstArr . fmap cborStr) diagnoses
      items <- sequence $ zipWith genIssuerSignedItem digestIDs $ zip (fmap cborKey healthReportKeys) $
        [ familyNameVal
        , givenNameVal
        , birthDateVal
        , issueDateVal
        , expiryDateVal
        , issuingAuthorityVal
        , diagnosesVal
        ]
      return items

genGoodDiagnoseSystem numBadDiagnosesAndGoodProviders numDiagnosesInReport maxlenDiagnosis
  = let
      go = do
             bads <- genRandomTests numBadDiagnosesAndGoodProviders maxlenDiagnosis
             reals <- genRandomTests numDiagnosesInReport maxlenDiagnosis
             if any (\ bad -> any (isPrefixOf bad) reals) bads
               then go
               else return (bads , reals)
    in
    go

main
  = do
      args <- getArgs
      guard (length args == 4) <|> fail "Expected exactly 4 args (output directory; the number of health reports; the number of diagnoses in each report; the number of bad diagnoses and recognized health care providers)"
      let outputDir = args !! 0
      let numHealthReports = read (args !! 1)
      let numDiagnosesInReport = read (args !! 2)
      let numBadDiagnosesAndGoodProviders = read (args !! 3)
      let fname = intercalate "x" (tail args)
      today <- fmap utctDay getCurrentTime
      let todayStr = formatShow iso8601Format today
      let issueDateVal = cborISO8601 . formatShow iso8601Format $ issued
      let expiryDateVal = cborISO8601 . formatShow iso8601Format $ addDays 180 today
      goodProviders <- genRandomTests numBadDiagnosesAndGoodProviders maxlenNpi
      (badDiagnoses , diagnoses) <- genGoodDiagnoseSystem numBadDiagnosesAndGoodProviders numDiagnosesInReport maxlenDiagnosis
      issuerSignedItemLists <- sequence $ genericReplicate numHealthReports $ fmap (fmap cbor) $ genHealthReportItems goodProviders diagnoses (issueDateVal , expiryDateVal)
      let healthReports = fmap (fmap (fmap (toEnum . fromIntegral))) issuerSignedItemLists
      let msos = fmap (cbor . flip genMSO (issueDateVal , issueDateVal , expiryDateVal)) issuerSignedItemLists
      publ <- return $ adapt $ JSONMapObj $ fromList $
        [ ("maxlen_npi" , JSONNumVal maxlenNpi)
        , ("maxlen_diagnosis" , JSONNumVal maxlenDiagnosis)
        , ("num_revealed_diagnoses" , JSONNumVal numRevealedDiagnoses)
        , ("family_name" , JSONStrVal familyName)
        , ("given_name" , JSONStrVal givenName)
        , ("dob" , JSONStrVal dob)
        , ("today" , JSONStrVal todayStr)
        , ("num_hrs" , JSONNumVal numHealthReports)
        , ("nums_diagnoses" , JSONArrVal (JSONLstArr (genericReplicate numHealthReports (JSONNumVal numDiagnosesInReport))))
        , ("num_bad_diagnoses" , JSONNumVal numBadDiagnosesAndGoodProviders)
        , ("num_good_providers" , JSONNumVal numBadDiagnosesAndGoodProviders)
        , ("lenss_hr_fields" , JSONArrVal (JSONLstArr (fmap (JSONArrVal . JSONLstArr . fmap (JSONNumVal . genericLength)) healthReports)))
        , ("num_hr_fields" , JSONNumVal (genericLength healthReportKeys))
        , ("num_mso_fields" , JSONNumVal (genericLength msoKeys))
        , ("num_isi_fields" , JSONNumVal (genericLength issuerSignedItemKeys))
        , ("lens_msos" , JSONArrVal (JSONLstArr (fmap (JSONNumVal . genericLength) msos)))
        ]
      inst <- return $ adapt $ JSONMapObj $ fromList $
        [ ("bad_diagnoses" , JSONArrVal (JSONLstArr (fmap JSONStrVal badDiagnoses)))
        , ("good_provider_npis" , JSONArrVal (JSONLstArr (fmap JSONStrVal goodProviders)))
        , ("revealed_diagnoses" , JSONArrVal (JSONLstArr (fmap JSONStrVal revealedDiagnoses)))
        , ("mso_hashes" , JSONArrVal (JSONLstArr (fmap (JSONStrVal . fmap (toEnum . fromIntegral) . digest) msos)))
        ]
      witn <- return $ adapt $ JSONMapObj $ fromList $ 
        [ ("hrs" , JSONArrVal (JSONLstArr (fmap (JSONArrVal . JSONLstArr . fmap JSONStrVal) healthReports)))
        , ("msos" , JSONArrVal (JSONLstArr (fmap (JSONStrVal . fmap (toEnum . fromIntegral)) msos)))
        ]
      createDirectoryIfMissing True outputDir
      writeFile (outputDir </> fname ++ "_public" <.> "json") $ dropWhile isSpace $ pretty 0 publ
      writeFile (outputDir </> fname ++ "_instance" <.> "json") $ dropWhile isSpace $ pretty 0 inst
      writeFile (outputDir </> fname ++ "_witness" <.> "json") $ dropWhile isSpace $ pretty 0 witn
